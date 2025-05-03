#ifndef SRC_FAKECAMERA_HPP_
#define SRC_FAKECAMERA_HPP_

#include <memory>
#include <opencv2/opencv.hpp>
#include <string>

#include "ICamera.hpp"

class FakeCamera : public ICamera {
 private:
  int h_res = 5472;
  int v_res = 3648;
  int seq = 1;

 public:
  FakeCamera() = default;  // Constructor

  void set_pixelformat(const std::string &pixelformat) override {}

  void set_exposuretime(float exposuretime) override {}

  void set_gain(float gain) override {}

  void enable_trigger(bool trigger_on) override {}

  void sensor_binning() override {
    h_res = h_res / 2;
    v_res = v_res / 2;
  };

  void output_pulse() override {}

  void start_stream(int num_buffers) override {}

  void stop_stream() override {}

  std::unique_ptr<ImageData> get_image(int timeout) override {
    std::unique_ptr<ImageData> image_data = std::make_unique<ImageData>();
    image_data->image = cv::Mat::zeros(v_res, h_res, CV_8UC1);
    image_data->seq = seq;
    seq++;

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
