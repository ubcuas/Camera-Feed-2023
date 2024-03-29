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
        Arena::CloseSystem(pSystem);
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
    std::cout << "Current epoch in milliseconds: " << epoch << "\n";
}

void CameraController::set_pixelformat(GenICam::gcstring pixelformat) {
    std::cout << "Setting pixel format to " << pixelformat << "\n";
    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "PixelFormat", pixelformat);
}

void CameraController::set_exposuretime(float exposuretime) {
    std::cout << "Setting auto exposure off\n";
    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ExposureAuto", "Off");

    std::cout << "Setting exposure time to" << exposuretime << "\n";
    GenApi::CFloatPtr pExposureTime = pDevice->GetNodeMap()->GetNode("ExposureTime");
	if (!pExposureTime || !GenApi::IsReadable(pExposureTime) || !GenApi::IsWritable(pExposureTime)) {
		throw GenICam::GenericException("ExposureTime node not found/readable/writable", __FILE__, __LINE__);
    }

	pExposureTime->SetValue(exposuretime);
}

void CameraController::set_gain(float gain) {
    std::cout << "Setting gain to " << gain << "\n";
    GenApi::CFloatPtr pGain = pDevice->GetNodeMap()->GetNode("Gain");
	if (!pGain || !GenApi::IsReadable(pGain) || !GenApi::IsWritable(pGain)) {
		throw GenICam::GenericException("Gain node not found/readable/writable", __FILE__, __LINE__);
    }

    pGain->SetValue(gain);
}

void CameraController::start_stream(int num_buffers) {
    std::cout << "Starting stream with " << num_buffers << " buffers\n";
    pDevice->StartStream(num_buffers);
}

void CameraController::stop_stream() {
    std::cout << "Stopping stream\n";
    pDevice->StopStream();
}

bool CameraController::get_image(Arena::IImage **pImage, long *timestamp) {
    try {
        *pImage = pDevice->GetImage(IMAGE_TIMEOUT);
        *timestamp = epoch + ((*pImage)->GetTimestampNs() / 1000000);
        std::cout << "Image captured\n";

        if ((*pImage)->IsIncomplete()) {
            std::cout << "Image incomplete\n";
            return false;
        }
        pDevice->RequeueBuffer(*pImage);
    } catch (GenICam::TimeoutException& ge) {
        return false;
    }

    return true;
}

void CameraController::set_default() {
    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetTLStreamNodeMap(), "StreamBufferHandlingMode", "NewestOnly");
    Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamAutoNegotiatePacketSize", true);
    Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamPacketResendEnable", true);

    set_pixelformat("BGR8");
}

void CameraController::cleanup() {
    std::cout << "Cleaning up\n";
    pSystem->DestroyDevice(pDevice);
    Arena::CloseSystem(pSystem);
}