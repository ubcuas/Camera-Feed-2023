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

#include <CLI/CLI.hpp>
#include <opencv2/opencv.hpp>

#include "ArenaApi.h"
#include "ICamera.hpp"
#include "ArenaCamera.hpp"
#include "CprHTTP.hpp"
#include "HttpTransmitter.hpp"
#include "TSQueue.hpp"

struct ImagePath {
  std::string path;
  int64_t timestamp;
};

struct EncodedData {
  std::unique_ptr<std::vector<uchar>> buf_ptr;
  int64_t timestamp;
};

static TSQueue<std::unique_ptr<ImageData>> data_queue;
static TSQueue<ImagePath> path_queue;
static TSQueue<EncodedData> encoded_queue;

std::atomic<bool> stop_flag(false);

void run(int seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  stop_flag = true;
  data_queue.abort();
  path_queue.abort();
  encoded_queue.abort();
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

// void image_saver(CameraController camera) {
//     while (!stop_flag) {
//         ImageData element;
//         try {
//             element = data_queue.pop();
//         } catch(const AbortedPopException& e) {
//             break;
//         }
//         Arena::IImage* pImage = element.pImage;
//         int64_t timestamp = element.timestamp;

//         // std::string filename = camera.save_image(pImage,
//         timestamp); std::future<std::string> future =
//         std::async(std::launch::async, &CameraController::save_image,
//         &camera, pImage, timestamp); std::string filename =
//         future.get(); ImagePath path = {filename, timestamp};
//         path_queue.push(path);
//         std::cout << "Saved " << filename << "\n";
//     }
// }

void image_processor() {
  // HttpTransmitter http_transmitter;
  HttpTransmitter http_transmitter;
  std::vector<int> compression_params;
  compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
  compression_params.push_back(100);  // Change the quality value (0-100), 100
                                      // is least compression and cpu usage

  std::string extension = ".jpg";

  while (!stop_flag) {
    ImageData element;
    try {
      // std::cout << id << " getting image\n";
      element = data_queue.pop();
      // std::cout << id << " got image\n";
    } catch (const AbortedPopException& e) {
      break;
    }
    Arena::IImage* pImage = element.pImage;
    int64_t timestamp = element.timestamp;

    // cv::Mat img = cv::Mat((int)pImage->GetHeight(), (int)pImage->GetWidth(),
    // CV_8UC1, const_cast<uint8_t*>(pImage->GetData()));
    cv::Mat mSource_Bayer(static_cast<int>(pImage->GetHeight()),
                          static_cast<int>(pImage->GetWidth()),
                          CV_8UC1,
                          const_cast<uint8_t*>(pImage->GetData()));
    cv::Mat mSource_Bgr(static_cast<int>(pImage->GetHeight()),
                        static_cast<int>(pImage->GetWidth()),
                        CV_8UC3);
    // cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);//Perform
    // demosaicing process

    std::vector<uchar> buf;
    std::shared_ptr<std::vector<uchar>> buf_ptr =
        std::make_shared<std::vector<uchar>>();

    cv::imencode(".jpg", mSource_Bayer, *buf_ptr, compression_params);
    Arena::ImageFactory::Destroy(pImage);
    // works till here lol

    // encoded_queue.push({std::move(buf_ptr), timestamp});
    std::cout << "Processed " << timestamp << "\n";
  }
}

void image_sender_imen(const std::string& url) {
  // HttpTransmitter http_transmitter;
  HttpTransmitter http_transmitter;
  std::vector<int> compression_params;
  compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
  compression_params.push_back(100);  // Change the quality value (0-100)

  std::string extension = ".jpg";

  while (!stop_flag) {
    EncodedData element;
    try {
      element = encoded_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }

    // Transfers ownership send_imen
    http_transmitter.send_imen(
        url, std::move(element.buf_ptr), element.timestamp);
    // (void) std::async(std::launch::async, &HttpTransmitter::send_imen,
    // &http_transmitter, url, &buf, timestamp);
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

int main(int argc, char* argv[]) {
  int seconds = 0;
  float exposureTime = 0;
  float gain = 0;
  std::string url = "";
  CLI::App app{"Camera Feed"};

  app.add_option("-s,--seconds", seconds, "Set runtime")->required();
  app.add_option("-e,--exposure", exposureTime, "Set exposure time (ms)")
      ->check(CLI::PositiveNumber);
  app.add_option("-g,--gain", gain, "Set gain")->check(CLI::PositiveNumber);
  app.add_option("-u,--url", url, "Set URL");

  CLI11_PARSE(app, argc, argv);

  std::shared_ptr<ICamera> camera = std::make_shared<ArenaCamera>();

  if (exposureTime != 0) {
    camera->set_exposuretime(exposureTime);
    std::cout << "Setting exposure time to " << exposureTime << "\n";
  }

  if (gain != 0) {
    camera->set_gain(exposureTime);
  }

  camera->start_stream();

  const int numProcessors = 1;
  // const int numSenders = 1;

  std::thread producer = std::thread(image_producer, camera);
  // cpu_set_t cpuset;
  // CPU_ZERO(&cpuset);
  // CPU_SET(0, &cpuset); // Change the core number as needed

  // int result = pthread_setaffinity_np(producer.native_handle(),
  // sizeof(cpu_set_t), &cpuset); if (result != 0) {
  //     std::cerr << "Error setting thread affinity: " << result << std::endl;
  // } else {
  //     std::cout << "Successfully set thread affinity to core 0" << std::endl;
  // }

  // // Set thread priority to maximum
  // struct sched_param param;
  // param.sched_priority = sched_get_priority_max(SCHED_FIFO);

  // // Set the scheduling policy and priority
  // if(pthread_setschedparam(producer.native_handle(), SCHED_FIFO, &param) !=
  // 0) {
  //     std::cerr << "Failed to set thread priority\n";
  //     return 1;
  // }

  std::cout << "CAMERA ONLINE\n";
  std::vector<std::thread> processors;
  for (int i = 0; i < numProcessors; i++) {
    processors.push_back(std::thread(image_processor));
  }

  // std::vector<std::thread> senders;
  // if (!url.empty()) {
  //     curl_global_init(CURL_GLOBAL_ALL);
  //     for (int i = 0; i < numSenders; i++) {
  //         senders.push_back(std::thread(image_sender_imen, url));
  //     }
  //     std::cout << "TRANSMITTER ONLINE\n";
  // }

  std::cout << "ALL SYSTEMS NOMINAL\n";
  run(seconds);

  producer.join();

  for (std::thread& processor : processors) {
    processor.join();
  }

  // if (!url.empty()) {
  //     for (std::thread& sender : senders) {
  //         sender.join();
  //     }
  // }

  camera->stop_stream();

  std::cout << "SYSTEM SHUTDOWN\n";
}
