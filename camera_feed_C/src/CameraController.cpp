#include "CameraController.h"
#include "ArenaApi.h"
// #include <stdio.h>
#include <chrono>
#include "SaveApi.h"


#define IMAGE_TIMEOUT 100

#define FILE_NAME_PATTERN "data/image<count>-<datetime:yyMMdd_hhmmss_fff>.jpg"



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

    	// get width, height, and pixel format nodes
    std::cout << "Configuring writer\n";
	GenApi::CIntegerPtr pWidth = pDevice->GetNodeMap()->GetNode("Width");
	GenApi::CIntegerPtr pHeight = pDevice->GetNodeMap()->GetNode("Height");
	GenApi::CEnumerationPtr pPixelFormat = pDevice->GetNodeMap()->GetNode("PixelFormat");

    Save::ImageParams params(
    static_cast<size_t>(pWidth->GetValue()),
    static_cast<size_t>(pHeight->GetValue()),
    Arena::GetBitsPerPixel(pPixelFormat->GetCurrentEntry()->GetValue()));

    writer.SetParams(params);
    writer.SetFileNamePattern(FILE_NAME_PATTERN);
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

void CameraController::set_trigger(bool trigger_on) {
    if (trigger_on) {
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

        trigger_state = true;
    } else {
        Arena::SetNodeValue<GenICam::gcstring>(
            pDevice->GetNodeMap(),
            "TriggerMode",
            "Off");
        trigger_state = false;
    }
}

void CameraController::set_acquisitionmode(GenICam::gcstring acq_mode) {
    	Arena::SetNodeValue<GenICam::gcstring>(
		pDevice->GetNodeMap(),
		"AcquisitionMode",
		acq_mode);
}


void CameraController::start_stream(int num_buffers) {
    std::cout << "Starting stream with " << num_buffers << " buffers\n";
    pDevice->StartStream(num_buffers);

    if (trigger_state) {
        std::cout << "Wait until trigger is armed\n";
        bool triggerArmed = false;

        do
        {
            triggerArmed = Arena::GetNodeValue<bool>(pDevice->GetNodeMap(), "TriggerArmed");
        } while (triggerArmed == false);  
    }
    
}

void CameraController::stop_stream() {
    std::cout << "Stopping stream\n";
    pDevice->StopStream();
}

bool CameraController::get_image(Arena::IImage **pImage, long *timestamp) {
    try {
        if (trigger_state) {
            Arena::ExecuteNode(
                pDevice->GetNodeMap(),
                "TriggerSoftware");
        }
        Arena::IImage *pBuffer = pDevice->GetImage(IMAGE_TIMEOUT);
        *timestamp = epoch + (pBuffer->GetTimestampNs() / 1000000);

        if (pBuffer->IsIncomplete()) {
            pDevice->RequeueBuffer(pBuffer);
            std::cout << "Image incomplete\n";
            return false;
        }

        *pImage = Arena::ImageFactory::Copy(pBuffer);
        pDevice->RequeueBuffer(pBuffer);
    } catch (GenICam::TimeoutException& ge) {
        return false;
    }
    return true;
}

void CameraController::save_image(Arena::IImage *pImage) {
    if (pImage->IsIncomplete()) {
        writer.SetFileNamePattern("data/INCOMPLETE-<datetime:yyMMdd_hhmmss_fff>-image<count>.jpg");
    } else {
        writer.SetFileNamePattern("data/<datetime:yyMMdd_hhmmss_fff>-image<count>.jpg");
    }
    writer.Save(pImage->GetData());
    std::cout << " at " << writer.GetLastFileName(true) << "\n";
    Arena::ImageFactory::Destroy(pImage);
}

void CameraController::set_default() {
    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetTLStreamNodeMap(), "StreamBufferHandlingMode", "NewestOnly");
    Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamAutoNegotiatePacketSize", true);
    Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamPacketResendEnable", true);
    // Arena::SetNodeValue<int64_t>(pDevice->GetNodeMap(), "DeviceLinkThroughputReserve", 30);  

    set_pixelformat("BGR8");
}

void CameraController::cleanup() {
    std::cout << "Cleaning up\n";
    pSystem->DestroyDevice(pDevice);
    Arena::CloseSystem(pSystem);
}