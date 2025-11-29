// Copyright 2024 UBC Uncrewed Aircraft Systems

#pragma once
#include <asio.hpp>
#include <memory>
#include "ISerialPort.hpp"

/**
 * @brief Wrapper around asio::serial_port that implements ISerialPort
 */
class RealSerialPort : public ISerialPort {
 public:
  explicit RealSerialPort(std::shared_ptr<asio::serial_port> serial_port)
      : serial_port_(serial_port) {}

  std::size_t read_some(asio::mutable_buffer buffer) override {
    return serial_port_->read_some(buffer);
  }

  std::size_t write_some(asio::const_buffer buffer) override {
    return asio::write(*serial_port_, buffer);
  }

  void set_option(const asio::serial_port_base::baud_rate& option) override {
    serial_port_->set_option(option);
  }

  void set_option(const asio::serial_port_base::character_size& option) override {
    serial_port_->set_option(option);
  }

  void set_option(const asio::serial_port_base::stop_bits& option) override {
    serial_port_->set_option(option);
  }

  void set_option(const asio::serial_port_base::parity& option) override {
    serial_port_->set_option(option);
  }

 private:
  std::shared_ptr<asio::serial_port> serial_port_;
};

