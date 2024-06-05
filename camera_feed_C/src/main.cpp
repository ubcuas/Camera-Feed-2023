#include "CameraController.h"
#include "ArenaApi.h"
#include "TSQueue.h"

#include <chrono>
#include <unistd.h>
#include <thread>         // std::this_thread::sleep_for
#include <condition_variable> 
#include <iostream> 
#include <mutex> 
#include <queue> 

struct ImagePath {
    std::string image_path;
    long timestamp;
};

struct ImageData {
    Arena::IImage* pImage;
    long timestamp;
};

TSQueue<ImageData> ImageQueue;
bool stopFlag = false;

void run(int seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stopFlag = true;
    ImageQueue.abort();
    std::cout << "DONE RUNNING\n";
}

long prod_start = 0;
long prod_end = 0;
void image_producer(CameraController camera_controller) {
    while (!stopFlag) {
        Arena::IImage* pImage;
        long timestamp;

        bool success = camera_controller.get_image(&pImage, &timestamp);
        if (success) {
            // Arena::ImageFactory::Destroy(pImage);
            ImageData data = {pImage, timestamp};
            // prod_end = timestamp;
            ImageQueue.push(data);
            // std::cout << "Pushed in: " << prod_end - prod_start << "\n";;
            // prod_start = timestamp;
        }
    }
}

long con_start = 0;
long con_end = 0;
void image_consumer(CameraController camera_controller) {
    try {
        while (!stopFlag) {
            ImageData data = ImageQueue.pop();
            Arena::IImage* pImage = data.pImage;
            long timestamp = data.timestamp;

            std::string filename = camera_controller.save_image(pImage, timestamp);
            con_end = timestamp;
            // Arena::ImageFactory::Destroy(pImage);
            std::cout << "Popped in: " << con_end - con_start << " " << filename << "\n";
            con_start = timestamp;
        }
    } catch(const AbortedPopException& e) {
        // exit
    }
}

int main(int argc, char *argv[]) {
    std::cerr << "Usage: " << argv[0] << " <seconds> <exposure_time_float>" << std::endl;
    CameraController camera_controller;
    int seconds = 1;

    if (argc >= 2) {
        seconds = std::stoi(argv[1]);
    }

    if (argc >= 3) {
        float exposureTime = std::stof(argv[2]);
        camera_controller.set_exposuretime(exposureTime);
    }

    // if (argc >= 4) {
    //     float gain = std::stof(argv[3]);
    //     camera_controller.set_gain(gain);
    // }
    // camera_controller.set_trigger(true);
    
    camera_controller.start_stream();

    const int numProducers = 1;
    const int numSavers = 8;
    
    std::vector<std::thread> producers;
    std::vector<std::thread> savers;
    
    for (int i = 0; i < numProducers; i++) {
        producers.push_back(std::thread(image_producer, camera_controller));
    }

    for (int i = 0; i < numSavers; i++) {
        savers.push_back(std::thread(image_consumer, camera_controller));
    }
    
    run(seconds);

    for (std::thread& saver : savers) {
        saver.join();
        std::cout << "Saver joined\n";
    }

    for (std::thread& producer : producers) {
        producer.join();
        std::cout << "Producer joined\n";
    }

    camera_controller.stop_stream();
}

// int main(int argc, char *argv[]) {
//     std::cerr << "Usage: " << argv[0] << " <num_images> <exposure_time_float> <gain_float>" << std::endl;
//     CameraController camera_controller;
//     int num_images = 1;

//     if (argc >= 2) {
//         num_images = std::stoi(argv[1]);
//     }

//     if (argc >= 3) {
//         float exposureTime = std::stof(argv[2]);
//         camera_controller.set_exposuretime(exposureTime);
//     }

//     if (argc >= 4) {
//         float gain = std::stof(argv[3]);
//         camera_controller.set_gain(gain);
//     }
//     // camera_controller.set_trigger(true);
    
//     camera_controller.start_stream();

//     Arena::IImage* pImage;
//     long timestamp;

//     int i = 0;
//     while (i < num_images) {
//         bool success = camera_controller.get_image(&pImage, &timestamp);
//         if (success) {
//             std::string filename = camera_controller.save_image(pImage);
//             // Arena::ImageFactory::Destroy(pImage);
//             std::cout << " at " << filename << " UNIX Timestamp: " << timestamp << "\n";
//             i++;
//         }
//     }

//     // // Get the current time point
//     // std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

//     // // Convert the time point to milliseconds since the epoch
//     // std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(
//     //     currentTime.time_since_epoch()
//     // );

//     // // Retrieve the count of milliseconds
//     // int64_t start = duration.count();

//     // long end = 0;
//     // for (int i = 0; i < 500; i++) {
//     //     bool success = camera_controller.get_image(&pImage, &timestamp, false);
//     //     end = timestamp;
//     //     if (!success) {
//     //         i--;
//     //     } else {
//     //         std::cout << "Image capture: " << i << " " << end - start << " " << success << "\n";
//     //         start = timestamp;
//     //     }
//     // }

//     // camera_controller.set_trigger(true);
    
//     // // Get the current time point
//     // currentTime = std::chrono::system_clock::now();

//     // // Convert the time point to milliseconds since the epoch
//     // duration = std::chrono::duration_cast<std::chrono::milliseconds>(
//     //     currentTime.time_since_epoch()
//     // );

//     // // Retrieve the count of milliseconds
//     // start = duration.count();

//     // end = 0;
//     // for (int i = 0; i < 500; i++) {
//     //     sleep(1);
//     //     bool success = camera_controller.get_image(&pImage, &timestamp, true);
//     //     end = timestamp;
//     //     if (!success) {
//     //         i--;
//     //     } else {
//     //         std::cout << "Image capture: " << i << " " << end - start << " " << success << "\n";
//     //         start = timestamp;
//     //     }
//     // }

//     camera_controller.stop_stream();
//     camera_controller.cleanup();
// }