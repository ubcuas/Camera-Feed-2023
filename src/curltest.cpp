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
// #include <mavsdk/mavsdk.h>
// #include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <chrono>


#include "Detector.hpp"
#include "projection.hpp"

#include "ardupilotmega/mavlink.h"

namespace fs = std::filesystem;
// using namespace mavsdk;
using namespace std::chrono_literals;


using asio::serial_port;
using asio::serial_port_base;
// #include "src/HttpTransmitter.hpp"

int main() {
  double roll_deg = 0.17000000178813934;
  double pitch_deg = 2.25;
  double yaw_deg = 285.8800048828125;
  double altitude = 0.0;
  double lat = 49.259629434902564;
  double lng = -123.24856966776673;
  std::cout << std::fixed << std::setprecision(15);

  std::pair<double, double> ul = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 0, 0, lat, lng);
  std::cout << ul.first << "," << ul.second << std::endl;

  std::pair<double, double> br = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 2736, 1824, lat, lng);
  std::cout << br.first << "," << br.second << std::endl;

  std::pair<double, double> ur = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 2736, 0, lat, lng);
  std::cout << ur.first << "," << ur.second << std::endl;

  std::pair<double, double> bl = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 0, 1824, lat, lng);
  std::cout << bl.first << "," << bl.second << std::endl;
  // double roll_deg = 10.0;
  // double pitch_deg = 10.0;
  // double yaw_deg = 10.0;
  // double altitude = 100.0;
  // double lat = 49.259629434902564;
  // double lng = -123.24856966776673;
  // std::cout << std::fixed << std::setprecision(15);

  // std::pair<double, double> ul = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 0, 0, lat, lng);
  // std::cout << ul.first << "," << ul.second << std::endl;

  // std::pair<double, double> br = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 2736, 1824, lat, lng);
  // std::cout << br.first << "," << br.second << std::endl;

  // std::pair<double, double> ur = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 2736, 0, lat, lng);
  // std::cout << ur.first << "," << ur.second << std::endl;

  // std::pair<double, double> bl = cam2Geoposition(roll_deg, pitch_deg, yaw_deg, altitude, 0, 1824, lat, lng);
  // std::cout << bl.first << "," << bl.second << std::endl;



}

// int main() {
//   // Initialize the serial port (adjust parameters as needed)
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
//   asio::io_context io_context;
//   asio::serial_port serial_port(io_context, matched_device);
//   serial_port.set_option(asio::serial_port_base::baud_rate(57600));
//   serial_port.set_option(asio::serial_port_base::character_size(8));
//   serial_port.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
//   serial_port.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));

//   try {
//     // MAVLink data buffers
//     std::vector<uint8_t> buffer(2048);
//     mavlink_message_t msg;
//     auto start_time = std::chrono::steady_clock::now();
    

//     while (true) {
//         auto now = std::chrono::steady_clock::now();
//         auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
//         if (elapsed.count() >= 1) {
//             break; // Exit after 1 second
//         }
//         // Read from serial port
//         std::size_t n = serial_port.read_some(asio::buffer(buffer));
//         // Process MAVLink message
//         for (std::size_t i = 0; i < n; ++i) {
//             if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
//                 if (msg.msgid == MAVLINK_MSG_ID_CAMERA_FEEDBACK) {
//                   mavlink_camera_feedback_t feedback;
//                   mavlink_msg_camera_feedback_decode(&msg, &feedback);

//                   std::cout << "📸 CAMERA_FEEDBACK purged:\n"
//                             << "  Time: " << feedback.time_usec << "\n"
//                             << "  Lat:  " << feedback.lat / 1e7 << "\n"
//                             << "  Lon:  " << feedback.lng / 1e7 << "\n"
//                             << "  Alt:  " << feedback.alt_msl << " m\n"
//                             << "  id:  " << feedback.img_idx << " m\n"
//                              << "  n:  " << feedback.completed_captures << " m\n"
//                             << std::endl;
//                 }
//             }
//         }
//     }

// } catch (const std::exception &e) {
//     std::cerr << "Error: " << e.what() << std::endl;
// }


//   mavlink_message_t msg;
//   uint8_t buf[MAVLINK_MAX_PACKET_LEN];

//   // Define the MAV_CMD_DO_DIGICAM_CONTROL command
//   uint8_t camera_id = 0;  // Example camera ID
//   uint8_t control_flags = 1; // 1 for take photo, 0 for other actions
//   float exposure_mode = 0;  // Placeholder for exposure mode
//   float trigger_mode = 1;   // Trigger camera

//   // Set the MAVLink message (MAV_CMD_DO_DIGICAM_CONTROL)
//   // mavlink_msg_command_long_pack(101, 101, &msg, 0, 0, MAV_CMD_DO_DIGICAM_CONTROL, 0, 
//   //                               0, 0, 0, 0, 1, 0, 0);
//   // mavlink_msg_command_long_pack(101, 101, &msg, 0, 0, MAV_CMD_IMAGE_START_CAPTURE, 0, 
//   //   0, 0.2, 9000, 0, 0, 0, 0);
//   // Serialize the message into buffer
//   uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

//   // Send the message over serial port
//   asio::write(serial_port, asio::buffer(buf, len));

//   std::cout << "Sent MAV_CMD_DO_DIGICAM_CONTROL message" << "\n";


  
//   try {
//       // MAVLink data buffers
//       std::vector<uint8_t> buffer(2048);
//       mavlink_message_t msg;
//       auto start_time = std::chrono::steady_clock::now();
      

//       while (true) {
//           auto now = std::chrono::steady_clock::now();
//           auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
//           if (elapsed.count() >= 3) {
//               break; // Exit after 1 second
//           }
//           // Read from serial port
//           std::size_t n = serial_port.read_some(asio::buffer(buffer));
//           // Process MAVLink message
//           for (std::size_t i = 0; i < n; ++i) {
//               if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
//                   if (msg.msgid == MAVLINK_MSG_ID_CAMERA_FEEDBACK) {
//                     mavlink_camera_feedback_t feedback;
//                     mavlink_msg_camera_feedback_decode(&msg, &feedback);

//                     std::cout << " CAMERA_FEEDBACK received:\n"
//                               << "  Time: " << feedback.time_usec << "\n"
//                               << "  Lat:  " << feedback.lat / 1e7 << "\n"
//                               << "  Lon:  " << feedback.lng / 1e7 << "\n"
//                               << "  Alt:  " << feedback.alt_msl << " m\n"
//                               << "  id:  " << feedback.img_idx << " m\n"
//                                << "  n:  " << feedback.completed_captures << " m\n"
//                               << std::endl;
//                   } else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK) {
//                     mavlink_command_ack_t ack;
//                     mavlink_msg_command_ack_decode(&msg, &ack);
//                     std::cout << "✅ COMMAND_ACK received:\n"
//                               << "  Command: " << ack.command << "\n"
//                               << "  Result:  " << (int)ack.result << "\n\n";
//                 }
//               }
//           }
//       }

//   } catch (const std::exception &e) {
//       std::cerr << "Error: " << e.what() << std::endl;
//   }

//   return 0;
// }


// int main() {
//   // Initialize the serial port (adjust parameters as needed)
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

//   Mavsdk mavsdk{Mavsdk::Configuration{ComponentType::GroundStation}};
//   ConnectionResult connection_result = mavsdk.add_any_connection("serial://" + matched_device + ":57600");
//   std::cout << "Waiting to discover system...\n";
//   auto prom = std::promise<std::shared_ptr<System>>{};
//   auto fut = prom.get_future();

//   // We wait for new systems to be discovered, once we find one that has a
//   // camera, we decide to use it.
//   Mavsdk::NewSystemHandle handle = mavsdk.subscribe_on_new_system([&mavsdk, &prom, &handle]() {
//       auto system = mavsdk.systems().back();

//       mavsdk.unsubscribe_on_new_system(handle);
//       prom.set_value(system);
//   });

//   // We usually receive heartbeats at 1Hz, therefore we should find a
//   // system after around 3 seconds max, surely.
//   if (fut.wait_for(3s) == std::future_status::timeout) {
//       std::cerr << "No system found, exiting.\n";
//       return 1;
//   }

//   // Get discovered system now.
//   auto system = fut.get();

//   MavlinkPassthrough mavlink_passthrough(system);

//   // MavlinkPassthrough::CommandLong command{
//   //   0,  // target system (usually 1)
//   //   0, // target component (usually 100 for camera)
//   //   MAV_CMD_DO_DIGICAM_CONTROL, // command
//   //   0, // session control (0 = stop, 1 = start)
//   //   0, // zoom
//   //   0, // zoom
//   //   0, // focus
//   //   1, // shoot (1 = trigger)
//   //   0, // command id (unused)
//   //   0  // extra
//   // };
//   MavlinkPassthrough::CommandLong command{
//     0, // target system (broadcast)
//     0, // target component (broadcast)
//     MAV_CMD_IMAGE_START_CAPTURE, // command
//     0, // camera id (broadcast)
//     1, // interval
//     100, // n images
//     0,
//     0,
//     0,
//     0
//   };

// //   mavlink_passthrough.subscribe_message(
// //     MAVLINK_MSG_ID_CAMERA_FEEDBACK,
// //     [](const mavlink_message_t& msg) {
// //         mavlink_camera_feedback_t feedback;
// //         mavlink_msg_camera_feedback_decode(&msg, &feedback);

// //         std::cout << "📸 CAMERA_FEEDBACK received:\n"
// //                   << "  Time: " << feedback.time_usec << "\n"
// //                   << "  Lat:  " << feedback.lat / 1e7 << "\n"
// //                   << "  Lon:  " << feedback.lng / 1e7 << "\n"
// //                   << "  Alt:  " << feedback.alt_msl << " m\n"
// //                   << std::endl;
// //     });

// // std::cout << "Listening for CAMERA_FEEDBACK messages..." << std::endl;


//   MavlinkPassthrough::Result msg_result = mavlink_passthrough.send_command_long(command);
//   if (msg_result != MavlinkPassthrough::Result::Success) {
//     std::cerr << "Taking Photo failed: " << msg_result;
//     return 1;
//   }

// //   std::cout << "MAV_CMD_DO_DIGICAM_CONTROL sent." << std::endl;
// //   mavlink_camera_feedback_t feedback;
  
  

// // // Keep the program running
// // while (true) {
// //     std::this_thread::sleep_for(std::chrono::seconds(1));
// // }


  
//   return 0;  
// }

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

// void demosaic_cpu(int iterations) {
//   cv::Mat mSource_Bayer = cv::Mat::zeros(3648, 5472, CV_8UC1);
//   cv::Mat mSource_Bgr(3648, 5472, CV_8UC1);

//   auto start = std::chrono::high_resolution_clock::now();

//   for (int i = 0; i < iterations; ++i) {
//     cv::cvtColor(mSource_Bayer, mSource_Bgr, cv::COLOR_BayerRG2BGR);
//   }

//   auto end = std::chrono::high_resolution_clock::now();
//   std::chrono::duration<double> elapsed = end - start;
//   double avg_time = elapsed.count() / iterations;
//   std::cout << "CPU demosaicing time: " << elapsed.count() << " seconds"
//             << "\n";
//   std::cout << "Average time per iteration: " << avg_time << " seconds"
//             << "\n";
// }

// void demosaic_gpu(int iterations) {
//   if (!cv::ocl::haveOpenCL()) {
//     std::cerr << "OpenCL is not available." << "\n";
//     return;
//   }

//   cv::ocl::setUseOpenCL(true);
//   cv::UMat img_gpu(3648, 5472, CV_8UC1);
//   cv::Mat img = cv::Mat::zeros(3648, 5472, CV_8UC1);
//   img.copyTo(img_gpu);
//   std::vector<std::future<void>> futures;

//   auto start = std::chrono::high_resolution_clock::now();

//   for (int i = 0; i < iterations; ++i) {
//     futures.push_back(std::async(std::launch::async, [&img_gpu]() {
//       cv::UMat bgr_img(3648, 5472, CV_8UC1);
//       cv::cvtColor(img_gpu, bgr_img, cv::COLOR_GRAY2BGR);
//     }));
//   }

//   for (auto& fut : futures) {
//     fut.get();
//   }

//   auto end = std::chrono::high_resolution_clock::now();
//   std::chrono::duration<double> elapsed = end - start;
//   double avg_time = elapsed.count() / iterations;
//   std::cout << "GPU demosaic time: " << elapsed.count() << " seconds"
//             << "\n";
//   std::cout << "Average time per iteration: " << avg_time << " seconds"
//             << "\n";
// }

// void tophat_gpu(int iterations) {

//   cv::UMat img_gpu(3648, 5472, CV_8UC1);
//   cv::Mat img = cv::Mat::zeros(3648, 5472, CV_8UC1);
//   img.copyTo(img_gpu);
//   cv::Mat kernel =
//   cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 13));
//   auto start = std::chrono::high_resolution_clock::now();

//   for (int i = 0; i < iterations; ++i) {
//     cv::UMat whitehat, mask;

//     // Apply White Hat transformation
//     cv::morphologyEx(img_gpu, whitehat, cv::MORPH_TOPHAT, kernel);
//   }


//   auto end = std::chrono::high_resolution_clock::now();
//   std::chrono::duration<double> elapsed = end - start;
//   double avg_time = elapsed.count() / iterations;
//   std::cout << "GPU tophat time: " << elapsed.count() << " seconds"
//             << "\n";
//   std::cout << "Average time per iteration: " << avg_time << " seconds"
//             << "\n";
// }

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


// int main() {
//   // Initialize the serial port (adjust parameters as needed)
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
//   asio::io_context io_context;
//   asio::serial_port serial_port(io_context, matched_device);
//   serial_port.set_option(asio::serial_port_base::baud_rate(115200));
//   serial_port.set_option(asio::serial_port_base::character_size(8));
//   serial_port.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
//   serial_port.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
  
//   try {


//       // MAVLink data buffers
//       std::vector<uint8_t> buffer(2048);
//       mavlink_message_t msg;

//       while (true) {
//           // Read from serial port
//           std::size_t n = serial_port.read_some(asio::buffer(buffer));
//           // Process MAVLink message
//           for (std::size_t i = 0; i < n; ++i) {
//               if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
//                   if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
//                     mavlink_heartbeat_t heartbeat;
//                     mavlink_msg_heartbeat_decode(&msg, &heartbeat);
//                     std::cout << "Received Heartbeat!\n";
//                     std::cout << "Type: " << static_cast<int>(heartbeat.type) << "\n";
//                     std::cout << "Autopilot: " << static_cast<int>(heartbeat.autopilot) << "\n";
//                     std::cout << "System Status: " << static_cast<int>(heartbeat.system_status) << "\n";
//                   }
//               }
//           }
//       }

//   } catch (const std::exception &e) {
//       std::cerr << "Error: " << e.what() << std::endl;
//   }

//   return 0;
// }