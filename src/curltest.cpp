// Copyright 2024 UBC Uncrewed Aircraft Systems

#include <unistd.h>
#include <curl/curl.h>

#include <iostream>
#include <thread>
#include <vector>

#include <CLI/CLI.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/opencv.hpp>

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
  std::cout << "CPU demosaicing time: " << elapsed.count() << " seconds" << std::endl;
  std::cout << "Average time per iteration: " << avg_time << " seconds" << std::endl;
}

void demosaic_gpu(int iterations) {
  if (!cv::ocl::haveOpenCL()) {
      std::cerr << "OpenCL is not available." << std::endl;
      return;
  }

  cv::ocl::setUseOpenCL(true);
  cv::UMat mSource_Bayer(3648, 5472, CV_8UC1, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
  cv::UMat mSource_Bgr(3648, 5472, CV_8UC3, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
  cv::Mat bayer = cv::Mat::zeros(3648, 5472, CV_8UC1);
  bayer.copyTo(mSource_Bayer);

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
      cv::cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  double avg_time = elapsed.count() / iterations;
  std::cout << "GPU demosaicing time: " << elapsed.count() << " seconds" << std::endl;
  std::cout << "Average time per iteration: " << avg_time << " seconds" << std::endl;
}

int main(int argc, char *argv[]) {
  int iterations = 10; // Default number of iterations
  CLI::App app{"Demosaic Performance Test"};
  app.add_option("-i,--iterations", iterations, "Number of iterations for performance test");
  CLI11_PARSE(app, argc, argv);

  std::cout << "Running CPU demosaicing..." << std::endl;
  demosaic_cpu(iterations);

  std::cout << "Running GPU demosaicing..." << std::endl;
  demosaic_gpu(iterations);

  return 0;
}
