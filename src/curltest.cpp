// Copyright 2024 UBC Uncrewed Aircraft Systems
#include <asio.hpp>

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
using asio::serial_port;
using asio::serial_port_base;
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

// void handle_heartbeat(const mavlink_message_t* message)
// {
//     mavlink_heartbeat_t heartbeat;
//     mavlink_msg_heartbeat_decode(message, &heartbeat);

//     printf("Got heartbeat from ");
//     switch (heartbeat.autopilot) {
//         case MAV_AUTOPILOT_GENERIC:
//             printf("generic");
//             break;
//         case MAV_AUTOPILOT_ARDUPILOTMEGA:
//             printf("ArduPilot");
//             break;
//         case MAV_AUTOPILOT_PX4:
//             printf("PX4");
//             break;
//         default:
//             printf("other");
//             break;
//     }
//     printf(" autopilot\n");
// }

// int main(int argc, char* argv[]) {
//   std::string device_prefix = "/dev/serial/by-id/";
//   std::regex pattern("usb-CubePilot_CubeOrange\\+_.*-if00");

//   // Search for the matching device
//   std::string matched_device;
//   try {
//     for (const auto& entry : fs::directory_iterator(device_prefix)) {
//         if (std::regex_match(entry.path().filename().string(), pattern)) {
//             matched_device = entry.path();
//             std::cout << "Found " + matched_device << "\n";
//             break;
//         }
//     }
//   } catch (const fs::filesystem_error& e) {
//       std::cout << "No USB devices detected: " << e.what() << "\n";
//       return 1;
//   }

//   if (matched_device.empty()) {
//     std::cout << "No matching device found." << "\n";
//     return 1;
//   }

//   // Create io_context and serial port objects
//   asio::io_context io;
//   serial_port serial(io, matched_device);
//   // close_serial_port(serial);

//   // // Try reopening the serial port
//   // serial.open(matched_device);


//   // Set the serial port options (ensure correct settings for ArduPilot)
//   serial.set_option(serial_port_base::baud_rate(115200));  // Example baud rate
//   serial.set_option(serial_port_base::character_size(8));
//   serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
//   serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
//   serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));


//   std::cout << "Connected to " << matched_device << " at 115200 baud\n";
//   std::cout << "Listening for MAVLink heartbeat messages...\n";
  
//   // Use a dynamic buffer (vector)
//   std::vector<uint8_t> buffer(2048);
  
//   while (true) {
//     // Read available bytes into the buffer
//     size_t len = asio::read(serial, asio::buffer(buffer.data(), buffer.size()));

//     mavlink_message_t message;
//     mavlink_status_t status;
//     for (int i = 0; i < len; i++) {
//         if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &message, &status) == 1) {
//             switch (message.msgid) {
//               case MAVLINK_MSG_ID_ATTITUDE:
//               mavlink_attitude_t attitude;
//               mavlink_msg_attitude_decode(&message, &attitude);
          
//               std::cout << "Roll: " << attitude.roll << " radians" << std::endl;
//               std::cout << "Pitch: " << attitude.pitch << " radians" << std::endl;
//               std::cout << "Yaw: " << attitude.yaw << " radians" << std::endl;
//             }
//         }
//     }
//   }



//   return 0;
// }


int main() {
  // Initialize the serial port (adjust parameters as needed)
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
  asio::io_context io_context;
  asio::serial_port serial_port(io_context, matched_device);
  serial_port.set_option(asio::serial_port_base::baud_rate(115200));
  serial_port.set_option(asio::serial_port_base::character_size(8));
  serial_port.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
  serial_port.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
  
  try {


      // MAVLink data buffers
      std::vector<uint8_t> buffer(2048);
      mavlink_message_t msg;

      while (true) {
          // Read from serial port
          std::size_t n = serial_port.read_some(asio::buffer(buffer));
          // Process MAVLink message
          for (std::size_t i = 0; i < n; ++i) {
              if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
                  if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                    mavlink_heartbeat_t heartbeat;
                    mavlink_msg_heartbeat_decode(&msg, &heartbeat);
                    std::cout << "Received Heartbeat!\n";
                    std::cout << "Type: " << static_cast<int>(heartbeat.type) << "\n";
                    std::cout << "Autopilot: " << static_cast<int>(heartbeat.autopilot) << "\n";
                    std::cout << "System Status: " << static_cast<int>(heartbeat.system_status) << "\n";
                  }
              }
          }
      }

  } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
  }

  return 0;
}