// Copyright 2024 UBC Uncrewed Aircraft Systems

#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>
#include <csignal>

#include <CLI/CLI.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <nlohmann/json.hpp>


#include "ArenaApi.h"
#include "ICamera.hpp"
#include "ArenaCamera.hpp"
#include "FakeCamera.hpp"
#include "HttpTransmitter.hpp"
#include "TSQueue.hpp"
#include "Pipeline.hpp"
#include "Detector.hpp"

using json = nlohmann::json;

static TSQueue<std::shared_ptr<ImageData>> data_queue;
static TSQueue<ImagePath> path_queue;
static TSQueue<std::shared_ptr<EncodedData>> encoded_queue;
static TSQueue<std::shared_ptr<ImageData>> save_queue;

static std::atomic<bool> stop_flag(false);
static std::mutex mtx;
static std::condition_variable cv_condition;

static bool save_img = false;


void signalHandler(int signum) {
  std::cout << "\nSIGINT received. Stopping...\n";
  stop_flag = true;
  cv_condition.notify_all();

  // Restore default handler for next SIGINT
  std::signal(SIGINT, SIG_DFL);

}

void run(int seconds) {
  std::unique_lock<std::mutex> lock(mtx);
  bool interrupted = cv_condition.wait_for(lock, std::chrono::seconds(seconds), [] {
      return stop_flag.load();
  });
  stop_flag = true;
  data_queue.abort();
  path_queue.abort();
  encoded_queue.abort();
  save_queue.abort();
  std::cout << "Aborting pop\n";
}

void image_producer(const std::shared_ptr<ICamera>& camera) {
  while (!stop_flag) {
    try {
      std::shared_ptr<ImageData> image_data = camera->get_image();
      data_queue.push(image_data);
      if (save_img) {
          save_queue.push(std::move(image_data));
      }

    } catch (timeout_exception& te) {
    }
  }
}

void image_saver() {
  std::vector<int> compression_params;
  compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
  compression_params.push_back(100);
  while (!stop_flag) {
    std::shared_ptr<ImageData> element;
    try {
      element = save_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }

    cv::Mat img = element->image;
    int64_t timestamp = element->timestamp;

    std::string filename =
        "images/" + std::to_string(element->timestamp) + ".jpg";
    cv::imwrite(filename, img, compression_params);
    std::cout << filename << "\n";
  }
}


void image_processor() {
  std::ofstream json_file("tag.txt", std::ios::app);

  while (!stop_flag) {
    std::shared_ptr<ImageData> element;
    try {
      element = data_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }

    cv::Mat img = element->image;

    cv::UMat img_gpu = img.getUMat(cv::ACCESS_READ);

    std::vector<cv::Point2d> points = predict_tophat(img_gpu);
    nlohmann::ordered_json j;
    j["TimeUS"] = element->timestamp;
    j["Img"] = element->seq;
    j["Points"] = json::array();
    for (const auto& pt : points) {
        j["Points"].push_back({pt.x, pt.y});
    }
    json_file << j.dump() << std::endl;
  }
  json_file.close();
}

void image_sender_imen(const std::string& url) {
  HttpTransmitter http_transmitter;

  while (!stop_flag) {
    std::shared_ptr<EncodedData> element;
    try {
      element = encoded_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }

    http_transmitter.send_imen(url, element);
  }
}

// void image_sender(std::string url) {
//     HttpTransmitter http_transmitter;
//     while (!stop_flag) {
//         ImagePath image_path;
//         try {
//             image_path = path_queue.pop();
//         } catch(const AbortedPopException& e) {
//             break;
//         }
//         std::string path = image_path.path;
//         long timestamp = image_path.timestamp;
//         (void) http_transmitter.send_imgfile(url, path, timestamp);
//         std::cout << "Sent " << path << "\n";
//     }
// }

bool setup_dir(std::string pathname) {
  if (!std::filesystem::exists(pathname)) {
    if (std::filesystem::create_directory(pathname)) {
      std::cout << "Directory created: " << pathname << "\n";
      return true;
    } else {
      std::cerr << "Failed to create directory: " << pathname << "\n";
      return false;
    }
  }
  return true;
}

int main(int argc, char* argv[]) {
  std::signal(SIGINT, signalHandler);

  int seconds = 0;
  float exposureTime = 0;
  float gain = 0;
  // bool reset = false;
  bool trigger = false;
  bool pulse = false;

  bool fake = false;
  bool bin = false;

  std::string url = "";
  CLI::App app{"Camera Feed"};

  auto runtime_opt = app.add_option("-s,--seconds", seconds, "Set runtime");
  runtime_opt->required();

  auto exposure_opt = app.add_option(
      "-e,--exposure", exposureTime, "Set camera exposure time (ms)");
  exposure_opt->check(CLI::PositiveNumber);

  auto gain_opt = app.add_option("-g,--gain", gain, "Set camera gain");
  gain_opt->check(CLI::PositiveNumber);

  // auto url_opt = app.add_option("-u,--url", url, "Set URL to send to");

  auto trigger_opt = app.add_flag("-t,--trigger", trigger, "Use trigger line 2");
  auto pulse_opt = app.add_flag("-p,--pulse", pulse, "Output pulse line 3");
  auto write_opt = app.add_flag("-w,--write", save_img, "Write images to disk");
  // auto reset_opt = app.add_flag("--reset", reset, "Reset camera to default");
  auto fake_opt = app.add_flag("-f,--fake", fake, "Use fake camera");
  auto bin_opt = app.add_flag("-b,--binning", bin, "Enable sensor binning");
  CLI11_PARSE(app, argc, argv);

  if (!cv::ocl::haveOpenCL()) {
    std::cerr << "OpenCL is not available." << "\n";
  } else {
    cv::ocl::setUseOpenCL(true);
  }


  if (save_img) {
    bool dir = setup_dir("images");
    if (!dir) {
      return 1;
    }
  }

  // if (url_opt->count() > 0) {
  //   send = true;
  // }
  std::shared_ptr<ICamera> camera;
  if (fake) {
    camera = std::make_shared<FakeCamera>();
  } else {
    camera = std::make_shared<ArenaCamera>();
  }

  if (trigger) {
    camera->enable_trigger(true);
  }

  if (pulse) {
    camera->output_pulse();
  }

  if (bin) {
    camera->sensor_binning();
  }

  if (exposure_opt->count() > 0) {
    camera->set_exposuretime(exposureTime);
    std::cout << "Setting exposure time to " << exposureTime << "\n";
  }

  if (gain_opt->count() > 0) {
    camera->set_gain(gain);
    std::cout << "Setting gain time to " << gain << "\n";
  }

  // if (reset){
  //   camera->set_default();
  // }

  camera->start_stream();

  const int numProcessors = 1;
  const int numSavers = 1;
  const int numSenders = 1;

  std::thread producer = std::thread(image_producer, camera);

  std::cout << "CAMERA ONLINE\n";
  std::vector<std::thread> processors;
  for (int i = 0; i < numProcessors; i++) {
    processors.push_back(std::thread(image_processor));
  }
  std::cout << "PROCESSOR ONLINE\n";

  std::vector<std::thread> savers;
  if (write) {
    for (int i = 0; i < numSavers; i++) {
      savers.push_back(std::thread(image_saver));
    }
    std::cout << "WRITER ONLINE\n";
  }

  // std::vector<std::thread> senders;
  // if (send) {
  //   curl_global_init(CURL_GLOBAL_ALL);
  //   for (int i = 0; i < numSenders; i++) {
  //     senders.push_back(std::thread(image_sender_imen, url));
  //   }
  //   std::cout << "TRANSMITTER ONLINE\n";
  // }

  // std::cout << "ALL SYSTEMS NOMINAL\n";
  run(seconds);

  producer.join();

  for (std::thread& processor : processors) {
    processor.join();
  }
  for (std::thread& saver : savers) {
    saver.join();
  }
  // for (std::thread& sender : senders) {
  //   sender.join();
  // }

  camera->stop_stream();

  std::cout << "SYSTEM SHUTDOWN\n";
}