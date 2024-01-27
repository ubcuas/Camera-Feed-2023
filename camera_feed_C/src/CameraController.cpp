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

    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "UserSetSelector", "Default");
    Arena::ExecuteNode(pDevice->GetNodeMap(), "UserSetLoad");

    set_epoch();
    set_default();

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

void CameraController::set_trigger(bool trigger_state) {
    if (trigger_state) {
        Arena::SetNodeValue<GenICam::gcstring>(
            pDevice->GetNodeMap(),
            "TriggerSelector",
            "FrameStart");

        // Set trigger mode
        //    Enable trigger mode before setting the source and selector and before
        //    starting the stream. Trigger mode cannot be turned on and off while the
        //    device is streaming.
        std::cout << "Enable trigger mode\n";

        Arena::SetNodeValue<GenICam::gcstring>(
            pDevice->GetNodeMap(),
            "TriggerMode",
            "On");

        // Set trigger source
        //    Set the trigger source to software in order to trigger images without
        //    the use of any additional hardware. Lines of the GPIO can also be used
        //    to trigger.
        std::cout << "Set trigger source to Software\n";

        Arena::SetNodeValue<GenICam::gcstring>(
            pDevice->GetNodeMap(),
            "TriggerSource",
            "Software");
    } else {
        Arena::SetNodeValue<GenICam::gcstring>(
            pDevice->GetNodeMap(),
            "TriggerMode",
            "Off");
    }
}

void CameraController::start_stream(int num_buffers) {
    std::cout << "Starting stream with " << num_buffers << " buffers\n";
    pDevice->StartStream(num_buffers);

    // std::cout << "Wait until trigger is armed\n";
	// bool triggerArmed = false;

    // do
	// {
	// 	triggerArmed = Arena::GetNodeValue<bool>(pDevice->GetNodeMap(), "TriggerArmed");
	// } while (triggerArmed == false);
}

void CameraController::stop_stream() {
    std::cout << "Stopping stream\n";
    pDevice->StopStream();
}

bool CameraController::get_image(Arena::IImage **pImage, long *timestamp, bool trigger_state) {
    try {
        if (trigger_state) {
            Arena::ExecuteNode(
                pDevice->GetNodeMap(),
                "TriggerSoftware");
        }
        Arena::IImage* testImage = pDevice->GetImage(IMAGE_TIMEOUT);
        *timestamp = epoch + (testImage->GetTimestampNs() / 1000000);
        // std::cout << "Image captured\n";

        if (testImage->IsIncomplete()) {
            std::cout << "Image incomplete\n";
            pDevice->RequeueBuffer(testImage);
            return false;
        } else {
            pDevice->RequeueBuffer(testImage);
        }
    } catch (GenICam::TimeoutException& ge) {
        // std::cout << "Image timeout\n";
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