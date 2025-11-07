// Copyright 2024 UBC Uncrewed Aircraft Systems

// Platform / POSIX
#include <pthread.h>
#include <unistd.h>

// C++ Standard Library
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <regex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Third-party libraries
#include <CLI/CLI.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <nlohmann/json.hpp>
#include <asio.hpp>

// Vendor /SDK
#include "ArenaApi.h"
#include "ardupilotmega/mavlink.h"

// Local project headers
#include "ICamera.hpp"
#include "ArenaCamera.hpp"
#include "FakeCamera.hpp"
#include "HttpTransmitter.hpp"
#include "TSQueue.hpp"
#include "Pipeline.hpp"
#include "Detector.hpp"
#include "projection.hpp"
#include "ISerialPort.hpp"
#include "RealSerialPort.hpp"
#include "FakeSerialPort.hpp"

// Macros / aliases / using
#define IMAGE_TIMEOUT 100

namespace fs = std::filesystem;
// using namespace mavsdk;
using namespace std::chrono_literals;

using asio::serial_port;
using asio::serial_port_base;

using json = nlohmann::json;

// Globals
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

// Handle SIGINT (Ctrl+C) and request shutdown
void signalHandler(int signum) {
  std::cout << "\nSIGINT received. Stopping...\n";
  stop_flag = true;
  cv_condition.notify_all();

  // Next SIGINT will trigger the default handler (immediate termination)
  std::signal(SIGINT, SIG_DFL);
}

// Wait up to `seconds` for a stop signal, then mark all queues aborted
void run(int seconds) {
  std::unique_lock<std::mutex> lock(mtx);

  bool interrupted = cv_condition.wait_for(
      lock, std::chrono::seconds(seconds), [] { return stop_flag.load(); });

  stop_flag = true;

  // Unblock any thread waiting on queue pops
  data_queue.abort();
  path_queue.abort();
  encoded_queue.abort();
  save_queue.abort();
  feedback_queue.abort();
  detect_queue.abort();

  std::cout << "Aborting pop\n";
}

// Continuously capture images and push them to processing (and save queue if
// enabled)
void image_producer(const std::shared_ptr<ICamera>& camera) {
  while (!stop_flag) {
    try {
      std::shared_ptr<ImageData> image_data = camera->get_image(IMAGE_TIMEOUT);
      data_queue.push(image_data);

      if (save_img) {
        save_queue.push(std::move(image_data));
      }

    } catch (timeout_exception& te) {
      // Ignore timeouts and continue
    }
  }
}

// Save images from the save_queue to disk as JPEGs
void image_saver() {
  std::vector<int> compression_params;
  compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
  compression_params.push_back(100);

  while (!stop_flag) {
    std::shared_ptr<ImageData> element;
    try {
      element = save_queue.pop();
    } catch (const AbortedPopException& e) {
      // Queue was aborted; exit thread
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

// Process images from data_queue: run detection and push results to
// detect_queue
void image_processor() {
  while (!stop_flag) {
    std::shared_ptr<ImageData> element;
    try {
      element = data_queue.pop();
    } catch (const AbortedPopException& e) {
      break;
    }

    cv::Mat img = element->image;

    // Move to UMat to allow OpenCL-accelerated processing if available
    cv::UMat img_gpu = img.getUMat(cv::ACCESS_READ);

    std::vector<cv::Point2d> points = predict_tophat(img_gpu);

    DetectData detect = {points, element->timestamp, element->seq};
    detect_queue.push(detect);
  }
}

// Continuously read MAVLink feedback messages from the serial port
void feedback_reader(std::shared_ptr<ISerialPort> serial_port) {
  std::vector<uint8_t> buffer(2048);
  mavlink_message_t msg;
  bool ack = false;

  while (!stop_flag) {
    // Previously used to trigger camera capture via MAV_CMD_IMAGE_START_CAPTURE
    // before reading CAMERA_FEEDBACK messages. This is now commented out
    // because image acquisition is handled directly by the camera API
    // (get_image()), and capture no longer depends on MAVLink control. Kept for
    // reference in case MAVLink-triggered capture is reinstated later.
    // ---------------------------------------------------------------------------
    // if (!ack) {
    //   uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    //   mavlink_msg_command_long_pack(101, 101, &msg, 0, 0,
    //   MAV_CMD_IMAGE_START_CAPTURE, 0, 0, 0.2, 9000, 0, 0, 0, 0);
    //   // Serialize the message into buffer
    //   uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    //   // Send the message over serial port
    //   asio::write(*serial_port, asio::buffer(buf, len));

    //   std::cout << "Sent MAV_CMD_IMAGE_START_CAPTURE message" << "\n";
    //   std::this_thread::sleep_for(std::chrono::seconds(1));
    // }

    // Read raw bytes from the serial port (might hang if disconnected)
    std::size_t n = serial_port->read_some(asio::buffer(buffer));

    // Parse MAVLink messages byte-by-byte
    for (std::size_t i = 0; i < n; i++) {
      if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
        // Only handle camera feedback messages
        if (msg.msgid == MAVLINK_MSG_ID_CAMERA_FEEDBACK) {
          mavlink_camera_feedback_t feedback;
          mavlink_msg_camera_feedback_decode(&msg, &feedback);

          ack = true;  // record that feedback has been seen
          feedback_queue.push(feedback);
        }
      }
    }
  }
}

// Pair detected image points with MAVLink camera feedback to produce
// geo-tagged output. Writes structured JSON records (tag.txt) and
// geoposition pairs (detect.csv). Synchronizes by matching detect.seq
// against feedback.img_idx with an offset id_diff.
void image_tagger(uint64_t sync_epoch, int64_t id_diff) {
  std::ofstream json_file("tag.txt", std::ios::app);
  std::ofstream csv_file("detect.csv",
                         std::ios::app);  // Open CSV file in append mode

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

    // Synchronize detect and feedback entries by sequence index.
    // Walk forward in whichever stream is behind.
    while (!stop_flag) {
      int64_t diff = detect.seq - feedback.img_idx - id_diff;

      if (diff == 0) {
        break;                // Synchronized
      } else if (diff < 0) {  // detect is behind
        std::cout << "detect is behind -> get next detect\n";
        try {
          detect = detect_queue.pop();
        } catch (const AbortedPopException&) {
          break;
        }
      } else {  // feedback is behind
        std::cout << "feedback is behind -> get next feedback\n";
        try {
          feedback = feedback_queue.pop();
        } catch (const AbortedPopException&) {
          break;
        }
      }
    }

    if (stop_flag) break;

    // Debug print showing raw MAVLink camera_feedback fields.
    // Left commented out to avoid console spam during normal operation,
    // but kept for troubleshooting / validation if feedback contents need
    // inspection.
    // ----------------------------------------------------------------------------------------
    // std::cout << "time_usec=" << feedback.time_usec << " target_system=" <<
    // static_cast<int>(feedback.target_system)
    //       << " cam_idx=" << static_cast<int>(feedback.cam_idx) << " img_idx="
    //       << feedback.img_idx
    //       << " lat=" << feedback.lat << " lng=" << feedback.lng << "
    //       alt_msl=" << feedback.alt_msl
    //       << " alt_rel=" << feedback.alt_rel << " roll=" << feedback.roll <<
    //       " pitch=" << feedback.pitch
    //       << " yaw=" << feedback.yaw << " foc_len=" << feedback.foc_len << "
    //       flags=" << feedback.flags
    //       << " completed_captures=" << feedback.completed_captures <<
    //       std::endl;
    int64_t delta_t = detect.timestamp - feedback.time_usec - sync_epoch;

    nlohmann::ordered_json j = {{"TimeUS", detect.timestamp},
                                {"Img", detect.seq},
                                {"Points", json::array()},
                                {"Epoch", sync_epoch},
                                {"Delta_t", delta_t}};

    for (const auto& pt : detect.points) {
      j["Points"].push_back({pt.x, pt.y});
    }

    j["Feedback"] = {{"time_usec", static_cast<uint64_t>(feedback.time_usec)},
                     {"img_idx", static_cast<uint16_t>(feedback.img_idx)},
                     {"lat", static_cast<int32_t>(feedback.lat)},
                     {"lng", static_cast<int32_t>(feedback.lng)},
                     {"alt_msl", static_cast<float>(feedback.alt_msl)},
                     {"alt_rel", static_cast<float>(feedback.alt_rel)},
                     {"roll", static_cast<float>(feedback.roll)},
                     {"pitch", static_cast<float>(feedback.pitch)},
                     {"yaw", static_cast<float>(feedback.yaw)},
                     {"completed_captures",
                      static_cast<uint16_t>(feedback.completed_captures)}};

    // Project each detected point into geographic coordinates and stream to CSV
    for (const auto& pt : detect.points) {
      std::pair<double, double> geo = cam2Geoposition(
          feedback.roll,
          feedback.pitch,
          feedback.yaw,
          feedback.alt_rel,
          pt.x,
          pt.y,
          static_cast<double>(feedback.lat) / 1e7,  // Convert to degrees
          static_cast<double>(feedback.lng) / 1e7   // Convert to degrees
      );
      csv_file << geo.first << "," << geo.second << std::endl;
    }

    json_file << j.dump() << std::endl;

    // Update sync reference
    sync_epoch = detect.timestamp - feedback.time_usec;
  }

  json_file.close();
  csv_file.close();
}

// Send encoded image data to the IMEN endopoint over HTTP
// Blocks on encodeed_queue and trasmits each entry until stop_flag is set
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

// Legacy image sender that transmitted image files from path_queue via HTTP.
// Replaced by image_sender_imen(), which sends encoded data directly rather
// than reading from disk. Left commented out for reference / fallback.
// -------------------------------------------------------------------------
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

// Ensure the given directory exists. Creates it if missing.
// Returns true if the directory exists or was successfully created, false on
// failure.
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

// Scan for a connected CubeOrange USB device and open a serial port to it.
// Returns a configured ISerialPort if found, nullptr otherwise.
std::shared_ptr<ISerialPort> connect(asio::io_context& io_context) {
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
    auto asio_serial_port =
        std::make_shared<asio::serial_port>(io_context, matched_device);
    auto serial_port = std::make_shared<RealSerialPort>(asio_serial_port);
    serial_port->set_option(asio::serial_port_base::baud_rate(57600));
    serial_port->set_option(asio::serial_port_base::character_size(8));
    serial_port->set_option(asio::serial_port_base::stop_bits(
        asio::serial_port_base::stop_bits::one));
    serial_port->set_option(
        asio::serial_port_base::parity(asio::serial_port_base::parity::none));
    return serial_port;
  } catch (const std::exception& e) {
    std::cout << "Failed to open serial port: " << e.what() << "\n";
    return nullptr;
  }
}

// Send a MAV_CMD_DO_DIGICAM_CONTROL command to the camera and collect any
// MAVLink CAMERA_FEEDBACK messages received for 1 second. Returns a vector
// of feedback messages.
std::vector<mavlink_camera_feedback_t> synchronize(
    std::shared_ptr<ISerialPort> serial_port) {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];

  // Pack the MAVLink command to control the camera (digital camera control)
  mavlink_msg_command_long_pack(
      101, 101, &msg, 0, 0, MAV_CMD_DO_DIGICAM_CONTROL, 0, 0, 0, 0, 0, 1, 0, 0);

  // Pack a MAVLink command to instruct the camera to start image capture.
  // Currently commented out because we're using MAV_CMD_DO_DIGICAM_CONTROL
  // instead.
  // ----------------------------------------------------------------------------------
  // mavlink_msg_command_long_pack(101, 101, &msg, 0, 0,
  // MAV_CMD_IMAGE_START_CAPTURE, 0,
  //   0, 0.2, 9000, 0, 0, 0, 0);

  // Serialize message to byte buffer
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

  // Send the message to the camera via serial port
  serial_port->write_some(asio::buffer(buf, len));
  std::cout << "Sent MAV_CMD_DO_DIGICAM_CONTROL message" << "\n";

  std::vector<mavlink_camera_feedback_t> feedbacks;
  std::vector<uint8_t> buffer(2048);
  auto start_time = std::chrono::steady_clock::now();

  while (true) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    if (elapsed.count() >= 1) {
      break;  // Stop reading after 1 second
    }

    // Read any bytes received from the camera
    std::size_t n = serial_port->read_some(asio::buffer(buffer));

    // Parse bytes for MAVLink messages
    for (std::size_t i = 0; i < n; i++) {
      if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, NULL)) {
        if (msg.msgid == MAVLINK_MSG_ID_CAMERA_FEEDBACK) {
          // Decode and store feedback message
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
  // Register Ctrl-C handler
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

  // CLI setup
  CLI::App app{"Camera Feed"};
  auto runtime_opt = app.add_option("-s,--seconds", seconds, "Set runtime");
  runtime_opt->required();

  auto exposure_opt = app.add_option(
      "-e,--exposure", exposureTime, "Set camera exposure time (ms)");
  exposure_opt->check(CLI::PositiveNumber);

  auto gain_opt = app.add_option("-g,--gain", gain, "Set camera gain");
  gain_opt->check(CLI::PositiveNumber);

  // auto url_opt = app.add_option("-u,--url", url, "Set URL to send to");

  auto trigger_opt =
      app.add_flag("-t,--trigger", trigger, "Use trigger line 2");
  auto pulse_opt = app.add_flag("-p,--pulse", pulse, "Output pulse line 3");
  auto write_opt = app.add_flag("-w,--write", save_img, "Write images to disk");
  // auto reset_opt = app.add_flag("--reset", reset, "Reset camera to default");
  auto fake_opt = app.add_flag("-f,--fake", fake, "Use fake camera");
  auto bin_opt = app.add_flag("-b,--binning", bin, "Enable sensor binning");
  auto auto_opt = app.add_flag(
      "-a,--auto", auto_trig, "Enable auto triggering (unreliable)");

  CLI11_PARSE(app, argc, argv);

  // Enable OpenCL if available
  if (!cv::ocl::haveOpenCL()) {
    std::cerr << "OpenCL is not available." << "\n";
  } else {
    cv::ocl::setUseOpenCL(true);
  }

  // Ensure images directory exists if saving images
  if (save_img) {
    bool dir = setup_dir("images");
    if (!dir) {
      return 1;
    }
  }

  // This block would enable sending images over the network if a URL was
  // provided. It is currently commented out because network sending is disabled
  // or not needed.
  // --------------------------------------------------------------------------------
  // if (url_opt->count() > 0) {
  //     send = true;
  // }
  asio::io_context io_context;
  std::shared_ptr<ISerialPort> serial_port = nullptr;

  // Connect to real or fake serial port
  if (!fake) {
    serial_port = connect(io_context);
    if (!serial_port) {
      std::cout << "No serial connection, exiting\n";
      return 1;
    }
  } else {
    serial_port = std::make_shared<FakeSerialPort>();
  }

  // Initialize camera (real or fake)
  std::shared_ptr<ICamera> camera;
  if (fake) {
    camera = std::make_shared<FakeCamera>();
  } else {
    camera = std::make_shared<ArenaCamera>();
  }

  if (trigger) camera->enable_trigger(true);
  if (pulse) camera->output_pulse();
  if (bin) camera->sensor_binning();

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

  // Synchronization with camera feedback
  std::vector<mavlink_camera_feedback_t> feedbacks;
  std::unique_ptr<ImageData> image_data;
  mavlink_camera_feedback_t feedback_msg;
  bool sync = false;
  int attempts = 0;

  /*
  This block handles synchronization for a fake camera.
  In fake mode, it skips the normal feedback-based sync and attempts to grab a
  single image immediately. It is commented out because the main loop now
  handles synchronization for both real and fake cameras.
  ------------------------------------------------------------------------------------------------------
  if (fake) {
      sync = true;
      std::cout << "Fake camera mode - skipping synchronization\n";
      try {
          image_data = camera->get_image(1000);
      } catch (timeout_exception& te) {
          std::cout << "Failed to get image from fake camera\n";
          camera->stop_stream();
          return 1;
      }
  } else {
 */

  // Attempt synchronization up to 3 times
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

    attempts++;
  }

  if (attempts >= 3 && !sync) {
    std::cout << "Failed to synchronize, exiting\n";
    camera->stop_stream();
    return 1;
  }
  //}

  // Calculate synchronizaiton epoch and image ID difference
  uint64_t sync_epoch = 0;
  uint64_t id_diff = 0;

  /*
  This block sets default synchronization values for a fake camera (sync_epoch
  and id_diff). It is commented out because the main synchronization logic now
  handles both fake and real cameras uniformly.
  ------------------------------------------------------------------------------------------------------------
  if (fake) {
      // For fake camera, set default values
      sync_epoch = 0;
      id_diff = 0;
      std::cout << "Fake camera mode - using default sync values\n";
  } else
   */

  // Calculate synchronization epoch and image ID difference
  if (sync && image_data && !feedbacks.empty()) {
    feedback_msg = feedbacks.back();
    sync_epoch = image_data->timestamp - feedback_msg.time_usec;
    id_diff = image_data->seq - feedback_msg.img_idx;
    std::cout << "Successfully synchronized with epoch: " << sync_epoch
              << "ID difference: " << id_diff << "\n";
  }

  if (seconds != 0) {
    // Thread setup: producer, processor, saver, feedback reader, tagger
    const int numProcessors = 1;
    const int numSavers = 1;
    const int numSenders = 1;

    // Producer
    std::thread producer = std::thread(image_producer, camera);
    std::cout << "CAMERA ONLINE\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Processor
    std::vector<std::thread> processors;
    for (int i = 0; i < numProcessors; i++) {
      processors.push_back(std::thread(image_processor));
    }
    std::cout << "PROCESSOR ONLINE\n";

    // Saver
    std::vector<std::thread> savers;
    if (write) {
      for (int i = 0; i < numSavers; i++) {
        savers.push_back(std::thread(image_saver));
      }
      std::cout << "WRITER ONLINE\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Feedback reader and image tagger
    std::thread reader = std::thread(feedback_reader, serial_port);
    std::thread tagger = std::thread(image_tagger, sync_epoch, id_diff);

    // Auto trigger camera if requested
    if (auto_trig) {
      mavlink_message_t msg;
      uint8_t buf[MAVLINK_MAX_PACKET_LEN];
      mavlink_msg_command_long_pack(101,
                                    101,
                                    &msg,
                                    0,
                                    0,
                                    MAV_CMD_IMAGE_START_CAPTURE,
                                    0,
                                    0,
                                    0.2,
                                    18000,
                                    0,
                                    0,
                                    0,
                                    0);
      uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
      serial_port->write_some(asio::buffer(buf, len));
      std::cout << "Sent MAV_CMD_IMAGE_START_CAPTURE message" << "\n";
    }

    // Previously used to start threads that upload images over HTTP.
    // Disabled because remote transmission is not needed right now,
    // but kept for optional future use.
    // --------------------------------------------------------------
    // std::vector<std::thread> senders;
    // if (send) {
    //   curl_global_init(CURL_GLOBAL_ALL);
    //   for (int i = 0; i < numSenders; i++) {
    //     senders.push_back(std::thread(image_sender_imen, url));
    //   }
    //   std::cout << "TRANSMITTER ONLINE\n";
    // }
    // std::cout << "ALL SYSTEMS NOMINAL\n";

    // Run main timer loop
    run(seconds);

    // Join threads
    producer.join();
    tagger.join();
    reader.join();

    for (std::thread& processor : processors) processor.join();
    for (std::thread& saver : savers) saver.join();
    // for (std::thread& sender : senders) sender.join();
  }

  camera->stop_stream();
  std::cout << "SYSTEM SHUTDOWN\n";
}
