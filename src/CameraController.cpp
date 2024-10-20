// Copyright 2024 UBC Uncrewed Aircraft Systems

#include "CameraController.hpp"

#include "ArenaApi.h"
// #include <stdio.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>

#include "SaveApi.h"

#define IMAGE_TIMEOUT 100

#define FILE_NAME_PATTERN "data/<timestampms>.raw"

/*
Should probably be a singleton, does not work with multiple devices
*/
CameraController::CameraController() {
  std::cout << "Connecting to camera\n";
  pSystem = Arena::OpenSystem();

  // Check for cameras
  pSystem->UpdateDevices(100);
  std::vector<Arena::DeviceInfo> deviceInfos = pSystem->GetDevices();
  if (deviceInfos.size() == 0) {
    Arena::CloseSystem(pSystem);
    throw std::runtime_error("No camera connected");
  }
  // Get the first camera
  pDevice = pSystem->CreateDevice(deviceInfos[0]);

  set_default();
  set_epoch();
  writer_config();
}

void CameraController::set_epoch() {
  std::cout << "Setting epoch\n";
  // Get camera timestamp in ns
  int64_t timestamp =
      Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "Timestamp");

  // Get time since epoch in ms
  std::chrono::system_clock::time_point currentTime =
      std::chrono::system_clock::now();

  std::chrono::milliseconds duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          currentTime.time_since_epoch());

  // Calculate epoch reference time
  epoch = duration.count() - (timestamp / 1000000);
  std::cout << "Current epoch in milliseconds: " << epoch << "\n";
}

void CameraController::set_default() {
  std::cout << "Setting default configuration\n";
  // Reset to default settings
  Arena::SetNodeValue<GenICam::gcstring>(
      pDevice->GetNodeMap(), "UserSetSelector", "Default");
  Arena::ExecuteNode(pDevice->GetNodeMap(), "UserSetLoad");

  Arena::SetNodeValue<GenICam::gcstring>(
      pDevice->GetTLStreamNodeMap(), "StreamBufferHandlingMode", "OldestFirst");
  Arena::SetNodeValue<bool>(
      pDevice->GetTLStreamNodeMap(), "StreamAutoNegotiatePacketSize", true);

  Arena::SetNodeValue<bool>(
      pDevice->GetTLStreamNodeMap(), "StreamPacketResendEnable", true);

  Arena::SetNodeValue<int64_t>(
      pDevice->GetNodeMap(), "DeviceLinkThroughputReserve", 10);

  // set_pixelformat("BGR8");
}

void CameraController::writer_config() {
  std::cout << "Configuring writer\n";
  // GenApi::CIntegerPtr pWidth = pDevice->GetNodeMap()->GetNode("Width");
  // GenApi::CIntegerPtr pHeight = pDevice->GetNodeMap()->GetNode("Height");
  // GenApi::CEnumerationPtr pPixelFormat =
  // pDevice->GetNodeMap()->GetNode("PixelFormat");

  // Save::ImageParams params(
  //     static_cast<size_t>(pWidth->GetValue()),
  //     static_cast<size_t>(pHeight->GetValue()),
  //     Arena::GetBitsPerPixel(pPixelFormat->GetCurrentEntry()->GetValue()));

  // writer.SetParams(params);
  writer.SetFileNamePattern(FILE_NAME_PATTERN);
}

void CameraController::set_pixelformat(GenICam::gcstring pixelformat) {
  // std::cout << "Setting pixel format to " << pixelformat << "\n";
  Arena::SetNodeValue<GenICam::gcstring>(
      pDevice->GetNodeMap(), "PixelFormat", pixelformat);
}

void CameraController::set_exposuretime(float exposuretime) {
  // std::cout << "Setting auto exposure off\n";
  Arena::SetNodeValue<GenICam::gcstring>(
      pDevice->GetNodeMap(), "ExposureAuto", "Off");

  GenApi::CFloatPtr pExposureTime =
      pDevice->GetNodeMap()->GetNode("ExposureTime");
  if (!pExposureTime || !GenApi::IsReadable(pExposureTime) ||
      !GenApi::IsWritable(pExposureTime)) {
    throw GenICam::GenericException(
        "ExposureTime node not found/readable/writable", __FILE__, __LINE__);
  }

  pExposureTime->SetValue(exposuretime);
}

void CameraController::set_gain(float gain) {
  std::cout << "Setting gain to " << gain << "\n";
  GenApi::CFloatPtr pGain = pDevice->GetNodeMap()->GetNode("Gain");
  if (!pGain || !GenApi::IsReadable(pGain) || !GenApi::IsWritable(pGain)) {
    throw GenICam::GenericException(
        "Gain node not found/readable/writable", __FILE__, __LINE__);
  }

  pGain->SetValue(gain);
}

void CameraController::set_trigger(bool trigger_on) {
  if (trigger_on) {
    Arena::SetNodeValue<GenICam::gcstring>(
        pDevice->GetNodeMap(), "TriggerSelector", "FrameStart");

    // Set trigger mode
    //    Enable trigger mode before setting the source and selector and before
    //    starting the stream. Trigger mode cannot be turned on and off while
    //    the device is streaming.
    std::cout << "Enable trigger mode\n";

    Arena::SetNodeValue<GenICam::gcstring>(
        pDevice->GetNodeMap(), "TriggerMode", "On");

    // Set trigger source
    //    Set the trigger source to software in order to trigger images without
    //    the use of any additional hardware. Lines of the GPIO can also be used
    //    to trigger.
    std::cout << "Set trigger source to Software\n";

    Arena::SetNodeValue<GenICam::gcstring>(
        pDevice->GetNodeMap(), "TriggerSource", "Software");

    trigger_state = true;
  } else {
    Arena::SetNodeValue<GenICam::gcstring>(
        pDevice->GetNodeMap(), "TriggerMode", "Off");

    trigger_state = false;
  }
}

void CameraController::set_acquisitionmode(GenICam::gcstring acq_mode) {
  Arena::SetNodeValue<GenICam::gcstring>(
      pDevice->GetNodeMap(), "AcquisitionMode", acq_mode);
}

void CameraController::start_stream(int num_buffers) {
  std::cout << "Starting stream with " << num_buffers << " buffers\n";
  pDevice->StartStream(num_buffers);

  if (trigger_state) {
    std::cout << "Wait until trigger is armed\n";
    bool triggerArmed = false;

    do {
      triggerArmed =
          Arena::GetNodeValue<bool>(pDevice->GetNodeMap(), "TriggerArmed");
    } while (triggerArmed == false);
  }
}

void CameraController::stop_stream() {
  std::cout << "Stopping stream\n";
  pDevice->StopStream();
}

bool CameraController::get_image(std::shared_ptr<cv::Mat>& image, int64_t* timestamp) {
  try {
    Arena::IImage* pImage = pDevice->GetImage(IMAGE_TIMEOUT);
    *timestamp = epoch + (pImage->GetTimestampNs() / 1000000);

    if (pImage->IsIncomplete()) {
      std::cout << "Image incomplete\n";
    }

    image = std::make_shared<cv::Mat>(cv::Mat(static_cast<int>(pImage->GetHeight()),
                            static_cast<int>(pImage->GetWidth()),
                            CV_8UC1,
                            const_cast<uint8_t*>(pImage->GetData()))
                        .clone());
    pDevice->RequeueBuffer(pImage);
  } catch (GenICam::TimeoutException& ge) {
    return false;
  }
  return true;
}

// std::string CameraController::save_image(Arena::IImage *pImage, int64_t
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

// void CameraController::get_statistics() {
//   // int missed_packets = Arena::GetNodeValue(pDevice->GetTLStreamNodeMap(),
//   // "StreamMissedPacketCount"); int missed_images =
//   // Arena::GetNodeValue(pDevice->GetTLStreamNodeMap(),
//   // "StreamMissedImageCount"); int lost_frames =
//   // Arena::GetNodeValue(pDevice->GetTLStreamNodeMap(), "StreamLostFrameCount");
//   // int frames = Arena::GetNodeValue(pDevice->GetTLStreamNodeMap(),
//   // "StreamStartedFrameCount"); int delivered_frames =
//   // Arena::GetNodeValue(pDevice->GetTLStreamNodeMap(),
//   // "StreamDeliveredFrameCount"); int fram_count =
//   // Arena::GetNodeValue(pDevice->GetTLStreamNodeMap(),
//   // "StreamStartedFrameCount"); int fram_count =
//   // Arena::GetNodeValue(pDevice->GetTLStreamNodeMap(),
//   // "StreamStartedFrameCount");
// }

void CameraController::cleanup() {
  std::cout << "Cleaning up\n";
  pSystem->DestroyDevice(pDevice);
  Arena::CloseSystem(pSystem);
}
