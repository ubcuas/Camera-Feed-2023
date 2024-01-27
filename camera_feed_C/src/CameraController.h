
#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <vector>
#include <stdexcept>
#include <ctime>

#include "ArenaApi.h"



// Include the necessary Arena headers here

class CameraController {
public:
    CameraController();
    void set_epoch();
    
    void set_pixelformat(GenICam::gcstring pixelformat);
    void set_exposuretime(float exposuretime);
    void set_gain(float gain);
    void set_trigger(bool trigger_state);
    void start_stream(int num_buffers = 10);
    void stop_stream();
    bool get_image(Arena::IImage **pImage, long *timestamp, bool trigger_state);
    void set_default();

    void cleanup();
    
private:
    Arena::ISystem* pSystem;
    Arena::IDevice* pDevice;
    int64_t epoch;

    int64_t SetIntValue(GenApi::INodeMap* pNodeMap, const char* nodeName, int64_t value);
};

#endif // CAMERA_CONTROLLER_H