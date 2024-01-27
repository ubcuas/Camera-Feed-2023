#include "CameraController.h"
#include "ArenaApi.h"
#include <chrono>
#include <unistd.h>
#include <thread>         // std::this_thread::sleep_for


int main() {
    CameraController camera_controller;

    camera_controller.set_default();
    camera_controller.start_stream();
    Arena::IImage* pImage;
    long timestamp;
    
    // Get the current time point
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

    // Convert the time point to milliseconds since the epoch
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime.time_since_epoch()
    );

    // Retrieve the count of milliseconds
    int64_t start = duration.count();

    long end = 0;
    for (int i = 0; i < 500; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        bool success = camera_controller.get_image(&pImage, &timestamp, false);
        end = timestamp;
        if (!success) {
            i--;
        } else {
            std::cout << "Image capture: " << i << " " << end - start << " " << success << "\n";
            start = timestamp;
        }
    }

    // camera_controller.set_trigger(true);
    
    // // Get the current time point
    // currentTime = std::chrono::system_clock::now();

    // // Convert the time point to milliseconds since the epoch
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    //     currentTime.time_since_epoch()
    // );

    // // Retrieve the count of milliseconds
    // start = duration.count();

    // end = 0;
    // for (int i = 0; i < 500; i++) {
    //     sleep(1);
    //     bool success = camera_controller.get_image(&pImage, &timestamp, true);
    //     end = timestamp;
    //     if (!success) {
    //         i--;
    //     } else {
    //         std::cout << "Image capture: " << i << " " << end - start << " " << success << "\n";
    //         start = timestamp;
    //     }
    // }

    camera_controller.stop_stream();
    camera_controller.cleanup();
}