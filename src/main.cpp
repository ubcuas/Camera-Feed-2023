// Copyright 2024 UBC Uncrewed Aircraft Systems

#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

#include <CLI/CLI.hpp>
#include <opencv2/opencv.hpp>

#include "ArenaApi.h"
#include "ICamera.hpp"
#include "ArenaCamera.hpp"
#include "FakeCamera.hpp"
#include "HttpTransmitter.hpp"
#include "TSQueue.hpp"
#include "Pipeline.hpp"

static TSQueue<std::unique_ptr<ImageData>> data_queue;
static TSQueue<ImagePath> path_queue;
static TSQueue<std::shared_ptr<EncodedData>> encoded_queue;
static TSQueue<std::shared_ptr<EncodedData>> save_queue;

std::atomic<bool> stop_flag(false);

void run(int seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
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
      std::unique_ptr<ImageData> image_data = camera->get_image();
      data_queue.push(std::move(image_data));
    } catch (timeout_exception& te) {
    }
  }
}

void image_saver() {
  while (!stop_flag) {
    std::shared_ptr<EncodedData> element;
    try {
      element = save_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }
    std::string filename =
        "images/" + std::to_string(element->timestamp) + ".jpg";

    std::ofstream outFile(filename);
    outFile.write(reinterpret_cast<const char*>(element->buf.data()),
                  element->buf.size());
    outFile.close();
  }
}

void image_processor(bool write, bool send) {
  std::vector<int> compression_params;
  compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
  compression_params.push_back(75);  // Change the quality value (0-100),
  // 100
  //                                     // is least compression and cpu usage

  while (!stop_flag) {
    std::unique_ptr<ImageData> element;
    try {
      element = data_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }
    cv::Mat mSource = element->image;
    int64_t timestamp = element->timestamp;

    // cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);//Perform
    if (write || send) {
      std::shared_ptr<EncodedData> encoded_img =
          std::make_shared<EncodedData>();
      encoded_img->timestamp = timestamp;
      cv::imencode(".jpg", mSource, encoded_img->buf, compression_params);

      if (write) {
        save_queue.push(encoded_img);
      }

      if (send) {
        encoded_queue.push(encoded_img);
        // std::cout << "Processed " << element->timestamp << "\n";
      }
    }
  }
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
  int seconds = 0;
  float exposureTime = 0;
  float gain = 0;
  // bool reset = false;
  bool write = false;
  bool send = false;
  std::string url = "";
  CLI::App app{"Camera Feed"};

  auto runtime_opt =
      app.add_option("-s,--seconds", seconds, "Set runtime")->required();
  auto exposure_opt =
      app.add_option(
             "-e,--exposure", exposureTime, "Set camera exposure time (ms)")
          ->check(CLI::PositiveNumber);
  auto gain_opt = app.add_option("-g,--gain", gain, "Set camera gain")
                      ->check(CLI::PositiveNumber);
  auto url_opt = app.add_option("-u,--url", url, "Set URL to send to");
  auto write_opt = app.add_flag("-w,--write", write, "Write images to disk");
  // auto reset_opt = app.add_flag("--reset", reset, "Reset camera to default");

  CLI11_PARSE(app, argc, argv);

  if (write) {
    bool dir = setup_dir("images");
    if (!dir) {
      return 1;
    }
  }

  if (url_opt->count() > 0) {
    send = true;
  }

  std::shared_ptr<ICamera> camera = std::make_shared<ArenaCamera>();

  if (exposure_opt->count() > 0) {
    camera->set_exposuretime(exposureTime);
    std::cout << "Setting exposure time to " << exposureTime << "\n";
  }

  if (gain_opt->count() > 0) {
    camera->set_gain(gain);
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
    processors.push_back(std::thread(image_processor, write, send));
  }
  std::cout << "PROCESSOR ONLINE\n";

  std::vector<std::thread> savers;
  if (write) {
    for (int i = 0; i < numSavers; i++) {
      savers.push_back(std::thread(image_saver));
    }
    std::cout << "WRITER ONLINE\n";
  }

  std::vector<std::thread> senders;
  if (send) {
    curl_global_init(CURL_GLOBAL_ALL);
    for (int i = 0; i < numSenders; i++) {
      senders.push_back(std::thread(image_sender_imen, url));
    }
    std::cout << "TRANSMITTER ONLINE\n";
  }

  std::cout << "ALL SYSTEMS NOMINAL\n";
  run(seconds);

  producer.join();

  for (std::thread& processor : processors) {
    processor.join();
  }
  for (std::thread& saver : savers) {
    saver.join();
  }
  for (std::thread& sender : senders) {
    sender.join();
  }

  camera->stop_stream();

  std::cout << "SYSTEM SHUTDOWN\n";
}
