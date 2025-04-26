// Copyright 2024 UBC Uncrewed Aircraft Systems

#include "ArenaCamera.hpp"

#include "ArenaApi.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include <opencv2/opencv.hpp>

#define IMAGE_TIMEOUT 100

/*
Should probably be a singleton, does not work with multiple devices
*/
ArenaCamera::ArenaCamera() {
  std::cout << "Connecting to camera\n";
  _pSystem = Arena::OpenSystem();

  // Check for cameras
  _pSystem->UpdateDevices(100);
  std::vector<Arena::DeviceInfo> deviceInfos = _pSystem->GetDevices();
  if (deviceInfos.size() == 0) {
    Arena::CloseSystem(_pSystem);
    throw std::runtime_error("No camera connected");
  }
  // Get the first camera
  _pDevice = _pSystem->CreateDevice(deviceInfos[0]);

  set_default();
  set_epoch();
}

ArenaCamera::~ArenaCamera() {
  std::cout << "Cleaning up\n";
  _pSystem->DestroyDevice(_pDevice);
  Arena::CloseSystem(_pSystem);
}

void ArenaCamera::set_pixelformat(const std::string& pixelformat) {
  // std::cout << "Setting pixel format to " << pixelformat << "\n";
  Arena::SetNodeValue<GenICam::gcstring>(
      _pDevice->GetNodeMap(), "PixelFormat", pixelformat.c_str());
}

void ArenaCamera::set_exposuretime(float exposuretime) {
  // std::cout << "Setting auto exposure off\n";
  Arena::SetNodeValue<GenICam::gcstring>(
      _pDevice->GetNodeMap(), "ExposureAuto", "Off");

  GenApi::CFloatPtr pExposureTime =
      _pDevice->GetNodeMap()->GetNode("ExposureTime");
  if (!pExposureTime || !GenApi::IsReadable(pExposureTime) ||
      !GenApi::IsWritable(pExposureTime)) {
    throw GenICam::GenericException(
        "ExposureTime node not found/readable/writable", __FILE__, __LINE__);
  }

  pExposureTime->SetValue(exposuretime);
}

void ArenaCamera::set_gain(float gain) {
  // std::cout << "Setting gain to " << gain << "\n";
  GenApi::CFloatPtr pGain = _pDevice->GetNodeMap()->GetNode("Gain");
  if (!pGain || !GenApi::IsReadable(pGain) || !GenApi::IsWritable(pGain)) {
    throw GenICam::GenericException(
        "Gain node not found/readable/writable", __FILE__, __LINE__);
  }

  pGain->SetValue(gain);
}

void ArenaCamera::enable_trigger(bool enable) {
  if (enable) {
    std::cout << "Trigger on\n";
    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "LineSelector", "Line2");
    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "LineMode", "Input");
    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "TriggerMode", "On");

    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "TriggerSelector", "FrameStart");
    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "TriggerMode", "On");
    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "TriggerSource", "Line2");

    _trigger_state = true;
  } else {
    std::cout << "Trigger off\n";
    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "TriggerMode", "Off");

    _trigger_state = false;
  }
}

void ArenaCamera::sensor_binning() {
  std::cout << "Sensor binning on\n";

  // Horizontal and vertical resolution halved, 4 pixels become 1
  Arena::SetNodeValue<GenICam::gcstring>(
    _pDevice->GetNodeMap(), "BinningSelector", "Sensor");
  Arena::SetNodeValue<int64_t>(
    _pDevice->GetNodeMap(), "BinningHorizontal", 2);
  Arena::SetNodeValue<int64_t>(
    _pDevice->GetNodeMap(), "BinningVertical", 2);
}



void ArenaCamera::start_stream(int num_buffers) {
  std::cout << "Starting stream with " << num_buffers << " buffers\n";
  _pDevice->StartStream(num_buffers);

  if (_trigger_state) {
    std::cout << "Wait until trigger is armed\n";
    bool triggerArmed = false;

    do {
      triggerArmed =
          Arena::GetNodeValue<bool>(_pDevice->GetNodeMap(), "TriggerArmed");
    } while (triggerArmed == false);
  }
}

void ArenaCamera::stop_stream() {
  // std::cout << "Stopping stream\n";
  _pDevice->StopStream();
}

std::unique_ptr<ImageData> ArenaCamera::get_image() {
  try {
    Arena::IImage* pImage = _pDevice->GetImage(IMAGE_TIMEOUT);

    if (pImage->IsIncomplete()) {
      std::cout << "Image incomplete\n";
    }
    std::unique_ptr<ImageData> image_data = std::make_unique<ImageData>();
    image_data->image = cv::Mat(static_cast<int>(pImage->GetHeight()),
                                static_cast<int>(pImage->GetWidth()),
                                CV_8UC1,
                                const_cast<uint8_t*>(pImage->GetData()))
                            .clone();
    image_data->timestamp = _epoch + (pImage->GetTimestampNs() / 1000000);
    image_data->seq = _seq;
    _seq++;
    _pDevice->RequeueBuffer(pImage);
    return image_data;
  } catch (GenICam::TimeoutException& ge) {
    throw timeout_exception("get_image() timed out");
  }
}


// void ArenaCamera::get_statistics() {
//   // int missed_packets = Arena::GetNodeValue(_pDevice->GetTLStreamNodeMap(),
//   // "StreamMissedPacketCount"); int missed_images =
//   // Arena::GetNodeValue(_pDevice->GetTLStreamNodeMap(),
//   // "StreamMissedImageCount"); int lost_frames =
//   // Arena::GetNodeValue(_pDevice->GetTLStreamNodeMap(),
//   "StreamLostFrameCount");
//   // int frames = Arena::GetNodeValue(_pDevice->GetTLStreamNodeMap(),
//   // "StreamStartedFrameCount"); int delivered_frames =
//   // Arena::GetNodeValue(_pDevice->GetTLStreamNodeMap(),
//   // "StreamDeliveredFrameCount"); int fram_count =
//   // Arena::GetNodeValue(_pDevice->GetTLStreamNodeMap(),
//   // "StreamStartedFrameCount"); int fram_count =
//   // Arena::GetNodeValue(_pDevice->GetTLStreamNodeMap(),
//   // "StreamStartedFrameCount");
// }

void ArenaCamera::set_epoch() {
  // Get camera timestamp in ns
  int64_t timestamp =
      Arena::GetNodeValue<int64_t>(_pDevice->GetNodeMap(), "Timestamp");

  // Get time since epoch in ms
  std::chrono::system_clock::time_point currentTime =
      std::chrono::system_clock::now();

  std::chrono::microseconds duration =
      std::chrono::duration_cast<std::chrono::microseconds>(
          currentTime.time_since_epoch());

  // Calculate _epoch reference time
  _epoch = duration.count() - (timestamp / 1000);
  std::cout << "Current epoch in microseconds: " << _epoch << "\n";
}

void ArenaCamera::set_default() {
  // Reset to default settings
  Arena::SetNodeValue<GenICam::gcstring>(
      _pDevice->GetNodeMap(), "UserSetSelector", "Default");
  Arena::ExecuteNode(_pDevice->GetNodeMap(), "UserSetLoad");

  Arena::SetNodeValue<GenICam::gcstring>(_pDevice->GetTLStreamNodeMap(),
                                         "StreamBufferHandlingMode",
                                         "OldestFirst");
  Arena::SetNodeValue<bool>(
      _pDevice->GetTLStreamNodeMap(), "StreamAutoNegotiatePacketSize", true);

  Arena::SetNodeValue<bool>(
      _pDevice->GetTLStreamNodeMap(), "StreamPacketResendEnable", true);

  Arena::SetNodeValue<int64_t>(
      _pDevice->GetNodeMap(), "DeviceLinkThroughputReserve", 10);
}
