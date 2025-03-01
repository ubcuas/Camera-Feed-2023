// Copyright 2024 UBC Uncrewed Aircraft Systems

#include "ArenaCamera.hpp"

#include "ArenaApi.h"
// #include <stdio.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>

#define IMAGE_TIMEOUT 100

#define FILE_NAME_PATTERN "data/<timestampms>.raw"

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
    Arena::SetNodeValue<GenICam::gcstring>(
        _pDevice->GetNodeMap(), "TriggerMode", "Off");

    _trigger_state = false;
  }
}

// void set_acquisitionmode(std::string acq_mode) {
//   // GenICam::gcstring a = acq_mode.c_str();
//   Arena::SetNodeValue<GenICam::gcstring>(
//       _pDevice->GetNodeMap(), "AcquisitionMode", acq_mode.c_str()
// }

void ArenaCamera::start_stream(int num_buffers) {
  std::cout << "Starting stream with " << num_buffers << " buffers\n";
  _pDevice->StartStream(num_buffers);

  // if (trigger_state) {
  //   std::cout << "Wait until trigger is armed\n";
  //   bool triggerArmed = false;

  //   do {
  //     triggerArmed =
  //         Arena::GetNodeValue<bool>(_pDevice->GetNodeMap(), "TriggerArmed");
  //   } while (triggerArmed == false);
  // }
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
    _pDevice->RequeueBuffer(pImage);
    return image_data;
  } catch (GenICam::TimeoutException& ge) {
    throw timeout_exception("get_image() timed out");
  }
}

// std::string ArenaCamera::save_image(Arena::IImage *pImage, int64_t
// timestamp) {
//     std::vector<int> compression_params;
//     compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
//     compression_params.push_back(100); // Change the quality value (0-100)

//     std::string extension = ".jpg";
//     std::string timestamp_str = std::to_string(timestamp);
//     std::string filename = timestamp_str + extension;

//     cv::Mat img = cv::Mat((int)pImage->GetHeight(), (int)pImage->GetWidth(),
//     CV_8UC3, (void *)pImage->GetData()); cv::imwrite(filename, img,
//     compression_params);

//     Arena::ImageFactory::Destroy(pImage);

//     return filename;
// }

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
  std::cout << "Setting epoch\n";
  // Get camera timestamp in ns
  int64_t timestamp =
      Arena::GetNodeValue<int64_t>(_pDevice->GetNodeMap(), "Timestamp");

  // Get time since epoch in ms
  std::chrono::system_clock::time_point currentTime =
      std::chrono::system_clock::now();

  std::chrono::milliseconds duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          currentTime.time_since_epoch());

  // Calculate _epoch reference time
  _epoch = duration.count() - (timestamp / 1000000);
  std::cout << "Current _epoch in milliseconds: " << _epoch << "\n";
}

void ArenaCamera::set_default() {
  std::cout << "Setting default configuration\n";
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

  // set_pixelformat("BGR8");
}
