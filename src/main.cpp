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
#include <regex>

#include <CLI/CLI.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <nlohmann/json.hpp>
#include <asio.hpp>


#include "ArenaApi.h"
#include "ICamera.hpp"
#include "ArenaCamera.hpp"
#include "FakeCamera.hpp"
#include "HttpTransmitter.hpp"
#include "TSQueue.hpp"
#include "Pipeline.hpp"
#include "Detector.hpp"

#include "ardupilotmega/mavlink.h"

#define IMAGE_TIMEOUT 100

namespace fs = std::filesystem;
// using namespace mavsdk;
using namespace std::chrono_literals;


using asio::serial_port;
using asio::serial_port_base;

using json = nlohmann::json;

static TSQueue<std::shared_ptr<ImageData>> data_queue;
static TSQueue<ImagePath> path_queue;
static TSQueue<std::shared_ptr<EncodedData>> encoded_queue;
static TSQueue<std::shared_ptr<ImageData>> save_queue;
static TSQueue<mavlink_camera_feedback_t> feedback_queue;
static TSQueue<DetectData> detect_queue;


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
  feedback_queue.abort();
  detect_queue.abort();
  std::cout << "Aborting pop\n";
}

void image_producer(const std::shared_ptr<ICamera>& camera) {
  while (!stop_flag) {
    try {
      std::shared_ptr<ImageData> image_data = camera->get_image(IMAGE_TIMEOUT);
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

    DetectData detect = {points, element->timestamp, element->seq};
    detect_queue.push(detect);
  }
}

void feedback_reader(std::shared_ptr<asio::serial_port> serial_port) {
  std::vector<uint8_t> buffer(2048);  
  mavlink_message_t msg;
  bool ack = false;
  
  while (!stop_flag) {
    // if (!ack) {
    //   uint8_t buf[MAVLINK_MAX_PACKET_LEN];
  
    //   mavlink_msg_command_long_pack(101, 101, &msg, 0, 0, MAV_CMD_IMAGE_START_CAPTURE, 0, 
    //   0, 0.2, 9000, 0, 0, 0, 0);
    //   // Serialize the message into buffer
    //   uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  
    //   // Send the message over serial port
    //   asio::write(*serial_port, asio::buffer(buf, len));
  
    //   std::cout << "Sent MAV_CMD_IMAGE_START_CAPTURE message" << "\n";
    //   std::this_thread::sleep_for(std::chrono::seconds(1));


    // }
    // Read from serial port (might hang if disconnected)
    std::size_t n = serial_port->read_some(asio::buffer(buffer));
    // Process MAVLink message
    for (std::size_t i = 0; i < n; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
            if (msg.msgid == MAVLINK_MSG_ID_CAMERA_FEEDBACK) {
              mavlink_camera_feedback_t feedback;
              mavlink_msg_camera_feedback_decode(&msg, &feedback);
              ack = true;
              feedback_queue.push(feedback);
            }
        }
    }
  }
}

void image_tagger(uint64_t sync_epoch, int64_t id_diff) {
  std::ofstream json_file("tag.txt", std::ios::app);

  while (!stop_flag) {
    DetectData detect;
    try {
      detect = detect_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }

    mavlink_camera_feedback_t feedback;
    bool synced = false;
    try {
      feedback = feedback_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }
    while (detect.seq - feedback.img_idx - id_diff != 0) {
      while (detect.seq - feedback.img_idx - id_diff < 0) {
        try {
          detect = detect_queue.pop();
        } catch (const AbortedPopException& e) {
          break;
        }
      }
      if (stop_flag) {
        break;
      }
      
      while (detect.seq - feedback.img_idx - id_diff > 0) {
        try {
          feedback = feedback_queue.pop();
        } catch (const AbortedPopException& e) {
          break;
        }
      }
      if (stop_flag) {
        break;
      }
    }
    
    if (stop_flag) {
      break;
    }


    // std::cout << "time_usec=" << feedback.time_usec << " target_system=" << static_cast<int>(feedback.target_system)
    //       << " cam_idx=" << static_cast<int>(feedback.cam_idx) << " img_idx=" << feedback.img_idx
    //       << " lat=" << feedback.lat << " lng=" << feedback.lng << " alt_msl=" << feedback.alt_msl
    //       << " alt_rel=" << feedback.alt_rel << " roll=" << feedback.roll << " pitch=" << feedback.pitch
    //       << " yaw=" << feedback.yaw << " foc_len=" << feedback.foc_len << " flags=" << feedback.flags
    //       << " completed_captures=" << feedback.completed_captures << std::endl;
    int64_t delta_t = detect.timestamp - feedback.time_usec - sync_epoch;

    nlohmann::ordered_json j = {
        {"TimeUS", detect.timestamp},
        {"Img", detect.seq},
        {"Points", json::array()},
        {"Epoch", sync_epoch},
        {"Delta_t", delta_t}
    };
    for (const auto& pt : detect.points) {
        j["Points"].push_back({pt.x, pt.y});
    }


    j["Feedback"] = {
      {"time_usec", static_cast<uint64_t>(feedback.time_usec)},
      {"img_idx", static_cast<uint16_t>(feedback.img_idx)},
      {"lat", static_cast<int32_t>(feedback.lat)},
      {"lng", static_cast<int32_t>(feedback.lng)},
      {"alt_msl", static_cast<float>(feedback.alt_msl)},
      {"alt_rel", static_cast<float>(feedback.alt_rel)},
      {"roll", static_cast<float>(feedback.roll)},
      {"pitch", static_cast<float>(feedback.pitch)},
      {"yaw", static_cast<float>(feedback.yaw)},
      {"completed_captures", static_cast<uint16_t>(feedback.completed_captures)}
    };
    
    json_file << j.dump() << std::endl;
    sync_epoch = detect.timestamp - feedback.time_usec;
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

std::shared_ptr<asio::serial_port> connect(asio::io_context& io_context) {
  std::string device_prefix = "/dev/serial/by-id/";
  std::regex pattern("usb-CubePilot_CubeOrange\\+_.*-if00");

  std::string matched_device;
  try {
      for (const auto& entry : fs::directory_iterator(device_prefix)) {
          if (std::regex_match(entry.path().filename().string(), pattern)) {
              matched_device = entry.path();
              std::cout << "Found " << matched_device << "\n";
              break;
          }
      }
  } catch (const fs::filesystem_error& e) {
      std::cout << "No USB devices detected: " << e.what() << "\n";
      return nullptr;
  }

  if (matched_device.empty()) {
      std::cout << "No matching device found." << "\n";
      return nullptr;
  }

  try {
      auto serial_port = std::make_shared<asio::serial_port>(io_context, matched_device);
      serial_port->set_option(asio::serial_port_base::baud_rate(57600));
      serial_port->set_option(asio::serial_port_base::character_size(8));
      serial_port->set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
      serial_port->set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
      return serial_port;
  } catch (const std::exception& e) {
      std::cout << "Failed to open serial port: " << e.what() << "\n";
      return nullptr;
  }
}



std::vector<mavlink_camera_feedback_t> synchronize(std::shared_ptr<asio::serial_port> serial_port) {
  
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];


  // Set the MAVLink message (MAV_CMD_DO_DIGICAM_CONTROL)
  mavlink_msg_command_long_pack(101, 101, &msg, 0, 0, MAV_CMD_DO_DIGICAM_CONTROL, 0, 
                                0, 0, 0, 0, 1, 0, 0);
  // mavlink_msg_command_long_pack(101, 101, &msg, 0, 0, MAV_CMD_IMAGE_START_CAPTURE, 0, 
  //   0, 0.2, 9000, 0, 0, 0, 0);
  // Serialize the message into buffer
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

  // Send the message over serial port
  asio::write(*serial_port, asio::buffer(buf, len));

  std::cout << "Sent MAV_CMD_DO_DIGICAM_CONTROL message" << "\n";

  std::vector<mavlink_camera_feedback_t> feedbacks;

  std::vector<uint8_t> buffer(2048);
  auto start_time = std::chrono::steady_clock::now();
  

  while (true) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    if (elapsed.count() >= 1) {
        break; // Exit after 1 second
    }
    // Read from serial port (might hang if disconnected)
    std::size_t n = serial_port->read_some(asio::buffer(buffer));
    // Process MAVLink message
    for (std::size_t i = 0; i < n; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
            if (msg.msgid == MAVLINK_MSG_ID_CAMERA_FEEDBACK) {
              mavlink_camera_feedback_t feedback;
              mavlink_msg_camera_feedback_decode(&msg, &feedback);

              feedbacks.push_back(feedback);
            }
        }
    }
  }
  return feedbacks;
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
  bool auto_trig = false;


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
  auto auto_opt = app.add_flag("-a,--auto", auto_trig, "Enable auto triggering (unreliable)");

  
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
  asio::io_context io_context;

  std::shared_ptr<asio::serial_port> serial_port = connect(io_context);
  if (!serial_port) {
    std::cout << "No serial connection, exiting\n";
    return 1;
  }
  
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
  // std::vector<mavlink_camera_feedback_t> feedback = synchr
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::vector<mavlink_camera_feedback_t> feedbacks;
  std::unique_ptr<ImageData> image_data;
  mavlink_camera_feedback_t feedback_msg;
  bool sync = false;
  int attempts = 0;
  while (!sync && attempts < 3) {
      std::cout << "Trying to synchronize...\n";
      std::this_thread::sleep_for(std::chrono::seconds(1));
      feedbacks = synchronize(serial_port);
      
      if (feedbacks.size() == 0) {
          std::cout << "No feedback detected\n";
      } else if (feedbacks.size() > 1) {
          std::cout << "Extra feedback detected, getting last\n";
          sync = true; 
      } else {  // feedbacks.size() == 1
          std::cout << "Feedback detected\n";
          sync = true;
      }
      
      try {
          image_data = camera->get_image(1000);
      } catch (timeout_exception& te) {
          sync = false;
          feedbacks.clear();
      }
      
      attempts++;  // Added increment
  }
  
  if (attempts >= 3 && !sync) {
      std::cout << "Failed to synchronize, exiting\n";
      camera->stop_stream();
      return 1;
  }
  
  uint64_t sync_epoch = 0;
  uint64_t id_diff = 0;
  if (sync && image_data && !feedbacks.empty()) {  // Added validation
      feedback_msg = feedbacks.back();
      sync_epoch = image_data->timestamp - feedback_msg.time_usec;
      id_diff = image_data->seq - feedback_msg.img_idx;
      std::cout << "Successfully synchronized with epoch: " << sync_epoch << "ID difference: " << id_diff <<"\n";
  }
  if (seconds != 0) {
    const int numProcessors = 1;
    const int numSavers = 1;
    const int numSenders = 1;
    std::thread producer = std::thread(image_producer, camera);
    std::cout << "CAMERA ONLINE\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));


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
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::thread reader = std::thread(feedback_reader, serial_port);
    std::thread tagger = std::thread(image_tagger, sync_epoch, id_diff);
    if (auto_trig) {
      mavlink_message_t msg;
      uint8_t buf[MAVLINK_MAX_PACKET_LEN];

      mavlink_msg_command_long_pack(101, 101, &msg, 0, 0, MAV_CMD_IMAGE_START_CAPTURE, 0, 
      0, 0.2, 18000, 0, 0, 0, 0);
      // Serialize the message into buffer
      uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

      // Send the message over serial port
      asio::write(*serial_port, asio::buffer(buf, len));

      std::cout << "Sent MAV_CMD_IMAGE_START_CAPTURE message" << "\n";
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

    tagger.join();
    reader.join();

    for (std::thread& processor : processors) {
      processor.join();
    }
    for (std::thread& saver : savers) {
      saver.join();
    }
    // for (std::thread& sender : senders) {
    //   sender.join();
    // }
  }

  camera->stop_stream();

  std::cout << "SYSTEM SHUTDOWN\n";
}