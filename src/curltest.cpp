// Copyright 2024 UBC Uncrewed Aircraft Systems

#include <curl/curl.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <future>
#include <iostream>
#include <opencv2/core/ocl.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>
#include <filesystem>
#include <regex>

#include "Detector.hpp"
#include "ardupilotmega/mavlink.h"
#include "common/mavlink.h"


namespace fs = std::filesystem;

// #include "src/HttpTransmitter.hpp"

// void image_sender(std::string url) {
//     HttpTransmitter http_transmitter;
//     while (1) {
//         std::vector<int> compression_params;

//         compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
//         compression_params.push_back(100); // Change the quality value
//         (0-100)

//         cv::Mat img = cv::imread("1721280101439.jpg", cv::IMREAD_COLOR);
//         std::vector<uchar> buf;
//         cv::imencode(".jpg", img, buf, compression_params);
//         // http_transmitter.send_imen(url, &buf, 1918183719895l);
//     }
// }

void demosaic_cpu(int iterations) {
  cv::Mat mSource_Bayer = cv::Mat::zeros(3648, 5472, CV_8UC1);
  cv::Mat mSource_Bgr(3648, 5472, CV_8UC1);

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    cv::cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  double avg_time = elapsed.count() / iterations;
  std::cout << "CPU demosaicing time: " << elapsed.count() << " seconds"
            << "\n";
  std::cout << "Average time per iteration: " << avg_time << " seconds"
            << "\n";
}

void demosaic_gpu(int iterations) {
  if (!cv::ocl::haveOpenCL()) {
    std::cerr << "OpenCL is not available." << "\n";
    return;
  }

  cv::ocl::setUseOpenCL(true);
  cv::UMat img_gpu(3648, 5472, CV_8UC1);
  cv::Mat img = cv::Mat::zeros(3648, 5472, CV_8UC1);
  img.copyTo(img_gpu);
  std::vector<std::future<void>> futures;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    futures.push_back(std::async(std::launch::async, [&img_gpu]() {
      cv::UMat bgr_img(3648, 5472, CV_8UC1);
      cv::cvtColor(img_gpu, bgr_img, cv::COLOR_GRAY2BGR);
    }));
  }

  for (auto& fut : futures) {
    fut.get();
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  double avg_time = elapsed.count() / iterations;
  std::cout << "GPU demosaic time: " << elapsed.count() << " seconds"
            << "\n";
  std::cout << "Average time per iteration: " << avg_time << " seconds"
            << "\n";
}

void tophat_gpu(int iterations) {

  cv::UMat img_gpu(3648, 5472, CV_8UC1);
  cv::Mat img = cv::Mat::zeros(3648, 5472, CV_8UC1);
  img.copyTo(img_gpu);
  cv::Mat kernel =
  cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 13));
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    cv::UMat whitehat, mask;

    // Apply White Hat transformation
    cv::morphologyEx(img_gpu, whitehat, cv::MORPH_TOPHAT, kernel);
  }


  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  double avg_time = elapsed.count() / iterations;
  std::cout << "GPU tophat time: " << elapsed.count() << " seconds"
            << "\n";
  std::cout << "Average time per iteration: " << avg_time << " seconds"
            << "\n";
}

// int main(int argc, char* argv[]) {
//   int iterations = 10;  // Default number of iterations
//   CLI::App app{"Demosaic Performance Test"};
//   app.add_option("-i,--iterations", iterations,
//                  "Number of iterations for performance test");
//   CLI11_PARSE(app, argc, argv);
//   cv::ocl::Context ctx = cv::ocl::Context::getDefault();
//   if (!ctx.ptr()) {
//       std::cerr << "OpenCL is not available" << std::endl;
//   }
//   cv::ocl::Device device = cv::ocl::Device::getDefault();
//   if (!device.compilerAvailable()) {
//       std::cerr << "OpenCL compiler is not available" << std::endl;
//   }

//   // std::cout << "Running CPU demosaicing..." << "\n";
//   // demosaic_cpu(iterations);

//   // std::cout << "Running GPU demosaicing..." << "\n";
//   // demosaic_gpu(iterations);
//   tophat_gpu(iterations);
//   cv::ocl::setUseOpenCL(true);
//   tophat_gpu(iterations);
//   return 0;
// }
int main(int argc, char* argv[]) {
  std::string device_prefix = "/dev/serial/by-id/";
  std::regex pattern("usb-CubePilot_CubeOrange\\+_.*-if00");

  // Search for the matching device
  std::string matched_device;
  try {
    for (const auto& entry : fs::directory_iterator(device_prefix)) {
        if (std::regex_match(entry.path().filename().string(), pattern)) {
            matched_device = entry.path();
            std::cout << "Found " + matched_device << "\n";
            break;
        }
    }
  } catch (const fs::filesystem_error& e) {
      std::cout << "No USB devices detected: " << e.what() << "\n";
      return 1;
  }

  if (matched_device.empty()) {
    std::cout << "No matching device found." << "\n";
    return 1;
  }

  return 0;
}