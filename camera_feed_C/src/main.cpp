#include "CameraController.h"
#include <chrono>

int main() {
    // Get the current time point
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

    // Convert the time point to milliseconds since the epoch
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime.time_since_epoch()
    );

    // Retrieve the count of milliseconds
    int64_t millis = duration.count();

    std::cout << "Current timestamp in milliseconds: " << millis << std::endl;
}