#include "CameraController.h"
#include "ArenaApi.h"
// #include <stdio.h>
#include <chrono>

#define IMAGE_TIMEOUT 100


CameraController::CameraController() {
    std::cout << "Connecting to camera\n";
    pSystem = Arena::OpenSystem();
    pSystem->UpdateDevices(100);

    std::vector<Arena::DeviceInfo> deviceInfos = pSystem->GetDevices();
    if (deviceInfos.size() == 0)
    {
        throw std::runtime_error("No camera connected");
    }
    pDevice = pSystem->CreateDevice(deviceInfos[0]);
}

void CameraController::set_epoch() {
    std::cout << "Setting epoch\n";
    // Get camera timestamp in ns
    int64_t timestamp = Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "Timestamp");

    // Get time since epoch in ms
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime.time_since_epoch()
    );

    // Calculate epoch reference time
    epoch = duration.count() - (timestamp / 1000000);
}

void CameraController::set_pixelformat(GenICam::gcstring pixelformat) {
    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "PixelFormat", pixelformat);
}

void CameraController::set_exposuretime(float exposuretime) {
    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ExposureAuto", "Off");

    GenApi::CFloatPtr pExposureTime = pDevice->GetNodeMap()->GetNode("ExposureTime");
	if (!pExposureTime || !GenApi::IsReadable(pExposureTime) || !GenApi::IsWritable(pExposureTime)) {
		throw GenICam::GenericException("ExposureTime node not found/readable/writable", __FILE__, __LINE__);
    }

	pExposureTime->SetValue(exposuretime);


}
void CameraController::set_gain(float gain) {
    GenApi::CFloatPtr pGain = pDevice->GetNodeMap()->GetNode("Gain");
	if (!pGain || !GenApi::IsReadable(pGain) || !GenApi::IsWritable(pGain)) {
		throw GenICam::GenericException("Gain node not found/readable/writable", __FILE__, __LINE__);
    }

    pGain->SetValue(gain);
}

void CameraController::start_stream(int num_buffers) {
    pDevice->StartStream(num_buffers);
}

void CameraController::stop_stream() {
    pDevice->StopStream();
}

bool CameraController::get_image(Arena::IImage *pImage, long *timestamp) {
    pImage = pDevice->GetImage(IMAGE_TIMEOUT);
    *timestamp = epoch + (pImage->GetTimestampNs() / 1000000);
    pDevice->RequeueBuffer(pImage);
    return true;
}

void CameraController::cleanup() {
    pSystem->DestroyDevice(pDevice);
    Arena::CloseSystem(pSystem);
}