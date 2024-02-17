
#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <vector>
#include <stdexcept>
#include <ctime>

#include "ArenaApi.h"
#include "SaveApi.h"




// Include the necessary Arena headers here

class CameraController {
public:
    CameraController();
    void set_epoch();
    
    void set_pixelformat(GenICam::gcstring pixelformat);
    void set_exposuretime(float exposuretime);
    void set_gain(float gain);
    void set_trigger(bool trigger_on);
    void set_acquisitionmode(GenICam::gcstring acq_mode);
    void start_stream(int num_buffers = 10);
    void stop_stream();
    bool get_image(Arena::IImage **pImage, long *timestamp);
    void save_image(Arena::IImage *pImage);
    void set_default();

    void cleanup();
    
private:
    Arena::ISystem* pSystem;
    Arena::IDevice* pDevice;
    Save::ImageWriter writer;
    int64_t epoch;
    bool trigger_state;

    int64_t SetIntValue(GenApi::INodeMap* pNodeMap, const char* nodeName, int64_t value);
};

#endif // CAMERA_CONTROLLER_H