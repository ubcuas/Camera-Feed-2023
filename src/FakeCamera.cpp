// Copyright 2024 UBC Uncrewed Aircraft Systems

#include "FakeCamera.hpp"

#include <chrono>
#include <thread>

#include <opencv2/opencv.hpp>

/*
Should probably be a singleton, does not work with multiple devices
*/
FakeCamera::FakeCamera() { std::cout << "Connecting to camera\n"; }

FakeCamera::~FakeCamera() { std::cout << "Cleaning up\n"; }

void FakeCamera::set_pixelformat(const std::string& pixelformat) {}

void FakeCamera::set_exposuretime(float exposuretime) {}

void FakeCamera::set_gain(float gain) {
  std::cout << "Setting gain to " << gain << "\n";
}

void FakeCamera::start_stream(int num_buffers) {
  std::cout << "Starting stream with " << num_buffers << " buffers\n";
}

void FakeCamera::stop_stream() { std::cout << "Stopping stream\n"; }

std::unique_ptr<ImageData> FakeCamera::get_image() {
  std::unique_ptr<ImageData> image_data = std::make_unique<ImageData>();
  image_data->image = cv::Mat::zeros(3648, 5472, CV_8UC1);

  std::chrono::system_clock::time_point currentTime =
      std::chrono::system_clock::now();

  std::chrono::milliseconds duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          currentTime.time_since_epoch());
  image_data->timestamp = duration.count();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  return image_data;
}
