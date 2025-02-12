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

// void demosaic_cpu() {
//   cv::Mat mSource_Bayer = cv::Mat::zeros(3648, 5472, CV_8UC1);

//   // Convert the RGB image to a single-channel grayscale image
//   while (1) {
//     cv::Mat mSource_Bgr;
//     cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);
//     usleep(200000);
//   }
// }

void demosaic_gpu() {
  // Ensure OpenCL is enabled and available
  if (!cv::ocl::haveOpenCL()) {
    std::cerr << "OpenCL is not available. Falling back to CPU." << std::endl;
    return;
  }

  cv::ocl::Context context;
  cv::ocl::Platform platform = cv::ocl::Platform::getDefault();
  
  // Get the active OpenCL device
  cv::ocl::Device device = cv::ocl::Device::getDefault();
  
  if (device.available()) {
      std::cout << "Device name: " << device.name() << std::endl;
      std::cout << "Device type: " << (device.type() == cv::ocl::Device::TYPE_GPU ? "GPU" : "Not GPU") << std::endl;
      std::cout << "Device memory: " << device.globalMemSize() / 1024 / 1024 << " MB" << std::endl;
  } else {
      std::cout << "No OpenCL device available." << std::endl;
  }

  // Set OpenCL context
  cv::ocl::setUseOpenCL(true);

  // Load the image into UMat (which will use GPU memory)
  cv::UMat mSource_Bayer;
  cv::Mat bayer = cv::Mat::zeros(3648, 5472, CV_8UC1);
  bayer.copyTo(mSource_Bayer);


  // Loop to continuously apply demosaic and convert Bayer to BGR
  while (1) {
    cv::UMat mSource_Bgr;

    cv::cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);
    std::cout << "HAHAHA\n";

  }
}

int main(int argc, char *argv[]) {
  demosaic_gpu();
  return 0;
}
