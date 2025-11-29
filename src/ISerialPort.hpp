// Copyright 2024 UBC Uncrewed Aircraft Systems

#pragma once
#include <asio.hpp>
#include <cstddef>
#include <memory>

/**
 * @brief Interface for serial port operations
 * Allows both real and fake serial ports to be used interchangeably
 */
class ISerialPort {
 public:
  virtual ~ISerialPort() = default;

  /**
   * @brief Read some data from the serial port
   * @param buffer Buffer to read data into
   * @return Number of bytes read
   */
  virtual std::size_t read_some(asio::mutable_buffer buffer) = 0;

  /**
   * @brief Write some data to the serial port
   * @param buffer Buffer containing data to write
   * @return Number of bytes written
   */
  virtual std::size_t write_some(asio::const_buffer buffer) = 0;

  /**
   * @brief Set the baud rate option
   */
  virtual void set_option(const asio::serial_port_base::baud_rate& option) = 0;

  /**
   * @brief Set the character size option
   */
  virtual void set_option(const asio::serial_port_base::character_size& option) = 0;

  /**
   * @brief Set the stop bits option
   */
  virtual void set_option(const asio::serial_port_base::stop_bits& option) = 0;

  /**
   * @brief Set the parity option
   */
  virtual void set_option(const asio::serial_port_base::parity& option) = 0;
};

