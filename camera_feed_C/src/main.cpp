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

bool stopFlag = false;

struct ImageData{
    std::string image_path;
    long timestamp;
};

// // Thread-safe queue https://www.geeksforgeeks.org/implement-thread-safe-queue-in-c/
// template <typename T> 
// class TSQueue { 
// private: 
//     // Underlying queue 
//     std::queue<T> m_queue; 
  
//     // mutex for thread synchronization 
//     std::mutex m_mutex; 
  
//     // Condition variable for signaling 
//     std::condition_variable m_cond; 
  
// public: 
//     // Pushes an element to the queue 
//     void push(T item) 
//     { 
//         // Acquire lock 
//         std::unique_lock<std::mutex> lock(m_mutex); 
  
//         // Add item 
//         m_queue.push(item); 
  
//         // Notify one thread that 
//         // is waiting 
//         m_cond.notify_one(); 
//     } 
  
//     // Pops an element off the queue 
//     T pop() 
//     { 
  
//         // acquire lock 
//         std::unique_lock<std::mutex> lock(m_mutex); 
  
//         // wait until queue is not empty 
//         m_cond.wait(lock, 
//                     [this]() { return !m_queue.empty(); }); 
  
//         // retrieve item 
//         T item = m_queue.front(); 
//         m_queue.pop(); 
  
//         // return item 
//         return item; 
//     } 

//     // Check if the queue is empty
//     bool empty() const
//     {
//         std::lock_guard<std::mutex> lock(m_mutex);
//         return m_queue.empty();
//     }

//     // Get the size of the queue
//     size_t size() const
//     {
//         std::lock_guard<std::mutex> lock(m_mutex);
//         return m_queue.size();
//     }
// }; 

void run(int seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds)); 
    stopFlag = true;
}

void image_producer(CameraController camera_controller) {
    while (!stopFlag) {
        Arena::IImage* pImage;
        long timestamp;

        bool success = camera_controller.get_image(&pImage, &timestamp);
        Arena::ImageFactory::Destroy(pImage);
    }
}

// void start_threads() {
//     // Number of consumer threads
//     const int numConsumers = 3;
    
//     // Container to hold consumer threads
//     std::vector<std::thread> consumers;
    
//     // Start consumer threads
//     for (int i = 0; i < numConsumers; ++i) {
//         consumers.emplace_back(consumerThread, i + 1);
//     }
    
//     // Enqueue some data (for demonstration purposes)
//     for (int i = 1; i <= 10; ++i) {
//         dataQueue.push(i);
//         // Simulate data production time
//         std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     }
    
//     // Join consumer threads
//     for (int i = 0; i < numConsumers; ++i) {
//         consumers[i].join();
//     }
// }

int main(int argc, char *argv[]) {
    CameraController camera_controller;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <exposure_time_float>" << std::endl;
        // return 1; // indicate error
    } else {
        float exposureTime = std::atof(argv[1]);


        camera_controller.set_exposuretime(exposureTime);

    }
    
    // std::cout << "Wait 30 seconds\n";
    // std::this_thread::sleep_for (std::chrono::seconds(30));
    camera_controller.start_stream();

    Arena::IImage* pImage;
    long timestamp;

    for (int i = 0; i < 20; i++) {
        bool success = camera_controller.get_image(&pImage, &timestamp);
        if (success) {
            camera_controller.save_image(pImage);
        }
    }
    
    // // Get the current time point
    // std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

    // // Convert the time point to milliseconds since the epoch
    // std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    //     currentTime.time_since_epoch()
    // );

    // // Retrieve the count of milliseconds
    // int64_t start = duration.count();

    // long end = 0;
    // for (int i = 0; i < 500; i++) {
    //     bool success = camera_controller.get_image(&pImage, &timestamp, false);
    //     end = timestamp;
    //     if (!success) {
    //         i--;
    //     } else {
    //         std::cout << "Image capture: " << i << " " << end - start << " " << success << "\n";
    //         start = timestamp;
    //     }
    // }

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