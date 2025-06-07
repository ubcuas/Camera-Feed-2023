#ifndef SRC_ARENACAMERA_HPP_
#define SRC_ARENACAMERA_HPP_

#include <memory>
#include <string>

#include "ArenaApi.h"
#include "ICamera.hpp"

class ArenaCamera : public ICamera {
 private:
  Arena::ISystem *_pSystem;    /**< Pointer to the system object. */
  Arena::IDevice *_pDevice;    /**< Pointer to the device object. */
  int64_t _epoch;              /**< Epoch time for timestamping. */
  bool _trigger_state = false; /**< Trigger state flag. */
  int32_t _seq = 1;

 public:
  ArenaCamera();   // Constructor
  ~ArenaCamera();  // Destructor

  void set_pixelformat(const std::string &pixelformat) override;

  void set_exposuretime(float exposuretime) override;

  void set_gain(float gain) override;

  void enable_trigger(bool trigger_on) override;

  void start_stream(int num_buffers = 10) override;

  void stop_stream() override;

  void sensor_binning() override;

  void output_pulse() override;

  std::unique_ptr<ImageData> get_image(int timeout) override;

 private:
  /**
   * @brief Sets default configurations for the camera.
   */
  void set_default();

  /**
   * @brief Sets the epoch time.
   */
  void set_epoch();
};

#endif  // SRC_ARENACAMERA_HPP_
