#pragma once
#include <asio.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>
#include <chrono>
/*
https://think-async.com/Asio/asio-1.11.0/doc/asio/reference/serial_port.html
*/
#include "ISerialPort.hpp"
#include "TSQueue.hpp"
#include "ardupilotmega/mavlink.h"

class FakeSerialPort : public ISerialPort {
 public:
  FakeSerialPort() = default;
  ~FakeSerialPort() {abort();}

  // mavlink camera feedback frames generated on demand
  std::size_t read_some(asio::mutable_buffer buffer) override {
    if (aborted_) return 0;
    if (pending.empty()) {
      generate_feedback_into_pending();
    }
    if (aborted_ || pending.empty()) return 0;
    const std::size_t n = std::min<std::size_t>(pending.size(), buffer.size());
    std::memcpy(buffer.data(), pending.data(), n);
    pending.erase(pending.begin(), pending.begin() + n);
    return n;
  }

  // Optional write, doesnt do anything in testing so should be synced with FakeCamera
  std::size_t write_some(asio::const_buffer buffer) override {
    return buffer.size();
  }

  void abort() {
    aborted_.store(true);
  }

  //These are just simulations for when they are called; doesnt do anything in testing
  void set_option(const asio::serial_port_base::baud_rate& o) override { baud_rate_ = o.value(); }
  void set_option(const asio::serial_port_base::character_size& o) override { char_size_ = o.value(); }
  void set_option(const asio::serial_port_base::stop_bits& o) override { /* no-op */ }
  void set_option(const asio::serial_port_base::parity& o) override { /* no-op */ }
  void set_period_ms(unsigned ms) { period_ms_ = ms; }
  void set_location_deg(double lat_deg, double lon_deg) {
    lat_e7_ = static_cast<int32_t>(lat_deg * 1e7);
    lon_e7_ = static_cast<int32_t>(lon_deg * 1e7);
  }
  void set_altitudes(float alt_msl, float alt_rel) { alt_msl_ = alt_msl; alt_rel_ = alt_rel; }
  void set_attitude_deg(float roll, float pitch, float yaw) { roll_deg_ = roll; pitch_deg_ = pitch; yaw_deg_ = yaw; }

 private:

  static inline uint64_t now_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
  }

  void generate_feedback_into_pending() {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    const uint64_t time_usec = now_us();
    mavlink_msg_camera_feedback_pack(
      1, 1, &msg,
      time_usec,
      0,                 // target_system
      0,                 // cam_idx
      img_idx_,          // img_idx
      lat_e7_,           // lat
      lon_e7_,           // lng
      alt_msl_,          // alt_msl
      alt_rel_,          // alt_rel
      roll_deg_,         // roll
      pitch_deg_,        // pitch
      yaw_deg_,          // yaw
      foc_len_,          // foc_len
      flags_,            // flags
      completed_captures_// completed_captures
    );

    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    pending.assign(buf, buf + len);

    // increment counters for next frame
    completed_captures_++;
    img_idx_++;
  }


  std::vector<uint8_t> pending;
  std::atomic<bool> aborted_{false};

  // Stored serial options (not used for actual serial port)
  unsigned baud_rate_ = 57600;
  unsigned char_size_ = 8;


  // Dummy mavlink_camera_feedback_t state
  uint16_t completed_captures_ = 0;
  uint16_t img_idx_ = 0;
  int32_t lat_e7_ = static_cast<int32_t>(1.1* 1e7);
  int32_t lon_e7_ = static_cast<int32_t>(-1.1 * 1e7);
  float alt_msl_ = 0.0f;
  float alt_rel_ = 100.0f;
  float roll_deg_ = 0.0f;
  float pitch_deg_ = 0.0f;
  float yaw_deg_ = 0.0f;
  float foc_len_ = 0.0f;
  uint8_t flags_ = 0;
  unsigned period_ms_ = 200;
};
