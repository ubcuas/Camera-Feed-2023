#ifndef SRC_FAKECAMERA_HPP_
#define SRC_FAKECAMERA_HPP_

#include "ICamera.hpp"
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>

class FakeCamera : public ICamera {
 public:
  FakeCamera() = default;   // Constructor
  
  void set_pixelformat(const std::string &pixelformat) override {}

  void set_exposuretime(float exposuretime) override {}

  void set_gain(float gain) override {}

  void enable_trigger(bool trigger_on) override {}

  void start_stream(int num_buffers) override {}

  void stop_stream() override {}

  std::unique_ptr<ImageData> get_image() override {
    std::unique_ptr<ImageData> image_data = std::make_unique<ImageData>();
    image_data->image = cv::imread("img.png", cv::IMREAD_UNCHANGED);
  
    std::chrono::system_clock::time_point currentTime =
        std::chrono::system_clock::now();
  
    std::chrono::microseconds duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            currentTime.time_since_epoch());
    image_data->timestamp = duration.count();
    std::this_thread::sleep_for(std::chrono::seconds(1));
  
    return image_data;
  }
};

#endif  // SRC_FAKECAMERA_HPP_
