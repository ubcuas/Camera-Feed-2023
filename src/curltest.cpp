#include <curl/curl.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <opencv2/core/ocl.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

#include "HttpTransmitter.h"

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

void demosaic_cpu() {
  cv::Mat im = cv::imread("1918183719895.jpg", cv::IMREAD_COLOR);
  cv::Mat mSource_Bayer;

  // Convert the RGB image to a single-channel grayscale image
  cv::cvtColor(im, mSource_Bayer, cv::COLOR_BGR2GRAY);
  while (1) {
    cv::Mat mSource_Bgr;
    cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);
    usleep(200000);
  }
}

void demosaic_gpu() {
  // Ensure OpenCL is enabled and available
  if (!cv::ocl::haveOpenCL()) {
    std::cerr << "OpenCL is not available. Falling back to CPU." << std::endl;
    return;
  }

  // Set OpenCL context
  cv::ocl::setUseOpenCL(true);

  // Load the image into UMat (which will use GPU memory)
  cv::UMat im;
  cv::imread("1918183719895.jpg", cv::IMREAD_COLOR).copyTo(im);

  cv::UMat mSource_Bayer;

  cv::cvtColor(im, mSource_Bayer, cv::COLOR_BGR2GRAY);

  // Loop to continuously apply demosaic and convert Bayer to BGR
  while (1) {
    cv::UMat mSource_Bgr;

    cv::cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);

    usleep(200000);  // Sleep for 0.2 seconds (200,000 microseconds)
  }
}

int main(int argc, char *argv[]) {
  demosaic_gpu();
  return 0;
}