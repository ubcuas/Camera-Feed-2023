#include <chrono>
#include <unistd.h>
#include <thread>
#include <condition_variable> 
#include <iostream> 
#include <mutex> 
#include <queue> 
#include <CLI/CLI.hpp>
#include <opencv2/opencv.hpp>

#include "CameraController.h"
#include "ArenaApi.h"
#include "TSQueue.h"
#include "HttpTransmitter.h"


struct ImagePath {
    std::string path;
    long timestamp;
};

struct ImageData {
    Arena::IImage* pImage;
    long timestamp;
};

TSQueue<ImageData> data_queue;
TSQueue<ImagePath> path_queue;

std::atomic<bool> stop_flag = ATOMIC_VAR_INIT(false);

void run(int seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop_flag = true;
    data_queue.abort();
    path_queue.abort();
    std::cout << "Aborting pop\n";
}
    // compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    // compression_params.push_back(100); // Change the quality value (0-100)

    // std::string extension = ".jpg";
    // std::string timestamp_str = std::to_string(timestamp);
    // std::string filename = timestamp_str + extension;

    // cv::Mat img = cv::Mat((int)pImage->GetHeight(), (int)pImage->GetWidth(), CV_8UC3, (void *)pImage->GetData());
    // // cv::imwrite(filename, img, compression_params);
    // std::vector<uchar> buf;

    // cv::imencode(".jpg", img, buf, compression_params);
void image_producer(CameraController camera_controller) {
    while (!stop_flag) {
        Arena::IImage* pImage;
        long timestamp;
        bool success = camera_controller.get_image(&pImage, &timestamp);
        if (success) {
            data_queue.push({pImage, timestamp});
        }
    }
}

void image_consumer(CameraController camera_controller) {
    while (!stop_flag) {
        ImageData data;
        try {
            data = data_queue.pop();
        } catch(const AbortedPopException& e) {
            break;
        }
        Arena::IImage* pImage = data.pImage;
        long timestamp = data.timestamp;

        std::string filename = camera_controller.save_image(pImage, timestamp);
        ImagePath path = {filename, timestamp};
        path_queue.push(path);
        std::cout << "Saved " << filename << "\n";
    }
}

void image_sender(std::string url) {
    HttpTransmitter http_transmitter(url);
    while (!stop_flag) {
        ImagePath image_path;
        try {
            image_path = path_queue.pop();
        } catch(const AbortedPopException& e) {
            break;
        }
        std::string path = image_path.path;
        long timestamp = image_path.timestamp;
        (void) http_transmitter.send(path, timestamp);
        std::cout << "Sent " << path << "\n";
    }
}   

int main(int argc, char *argv[]) {
    std::cout << "help me\n";
    int seconds = 0;
    float exposureTime = 0;
    float gain = 0;
    std::string url = "";
    CLI::App app{"CLI Example"};

    app.add_option("-s,--seconds", seconds, "Set runtime")->required();
    app.add_option("-e,--exposure", exposureTime, "Set exposure time (ms)")->check(CLI::PositiveNumber);
    app.add_option("-g,--gain", gain, "Set gain")->check(CLI::PositiveNumber);
    app.add_option("-u,--url", url, "Set URL");

    CLI11_PARSE(app, argc, argv);

    std::cout << seconds << '\n';
    std::cout << exposureTime << '\n';
    std::cout << gain << '\n';
    std::cout << url << '\n';
    
    CameraController camera_controller;

    if (exposureTime != 0) {
        camera_controller.set_exposuretime(exposureTime);
    }

    if (gain != 0) {
        camera_controller.set_gain(exposureTime);
    }

    // camera_controller.set_trigger(true);
    
    camera_controller.start_stream();


    const int numProducers = 1;
    const int numSavers = 1;
    const int numSenders = 2;

    
    std::vector<std::thread> producers;
    std::vector<std::thread> savers;
    std::vector<std::thread> senders;
    
    for (int i = 0; i < numProducers; i++) {
        producers.push_back(std::thread(image_producer, camera_controller));
    }
    std::cout << "CAMERA ONLINE\n";

    for (int i = 0; i < numSavers; i++) {
        savers.push_back(std::thread(image_consumer, camera_controller));
    }
    std::cout << "WRITER ONLINE\n";

    if (!url.empty()) {
        for (int i = 0; i < numSenders; i++) {
            senders.push_back(std::thread(image_sender, url));
        }
        std::cout << "TRANSMITTER ONLINE\n";

    }
    
    std::cout << "ALL SYSTEMS NOMINAL\n";
    run(seconds);

    for (std::thread& producer : producers) {
        producer.join();
    }
    std::cout << "Producers joined\n";

    for (std::thread& saver : savers) {
        saver.join();
    }
    std::cout << "Savers joined\n";

    if (!url.empty()) {
        for (std::thread& sender : senders) {
            sender.join();
        }
        std::cout << "Senders joined\n";
    }

    camera_controller.stop_stream();
    camera_controller.cleanup();

    std::cout << "SYSTEM SHUTDOWN\n";
}