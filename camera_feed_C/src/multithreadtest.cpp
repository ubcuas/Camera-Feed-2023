#include "TSQueue.h"
#include <vector>
#include <thread>




bool stopFlag = false;

TSQueue<int> ImageQueue;
TSQueue<int> TagQueue;

int image_num = 0;

void run(int seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds)); 
    stopFlag = true;
    std::cout << "DONE RUNNING\n";
}


void producer() {
    while (!stopFlag) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); 
        image_num++;
        ImageQueue.push(image_num);
        // std::cout << "Pushed " << image_num << "\n";
    }
}

void saver() {
    while (!stopFlag) {
        int r = ImageQueue.pop();
        std::this_thread::sleep_for(std::chrono::seconds(3)); 
        // std::cout << "Popped " << r << "\n";
    }
}

void printer() {
    while (!stopFlag) {
        std::cout << ImageQueue.size() << "\r";
    }
    std::cout << "\n";
}

// void tagger(int j) {
//     while (!stopFlag) {
//         int r = TagQueue.pop();
//         std::this_thread::sleep_for(std::chrono::seconds(2)); 
//         std::cout << " Popped " << r << "\n";
//     }
// }

// void Saver(int j) {
//     while (!stopFlag) {
//         int r = queue.pop();
//         std::this_thread::sleep_for(std::chrono::seconds(2)); 
//         std::cout << " Popped " << r << "\n";
//     }
// }

void start_threads() {
    const int numProducers = 2;
    const int numSavers = 4;
    const int numTaggers = 1;
    
    std::vector<std::thread> producers;
    std::vector<std::thread> savers;
    // std::vector<std::thread> Taggers;

    for (int i = 0; i < numProducers; i++) {
        savers.push_back(std::thread(producer));
    }

    
    for (int i = 0; i < numSavers; i++) {
        savers.push_back(std::thread(saver));
    }
    std::thread help(printer);

    // for (int i = 0; i < numTaggers; ++i) {
    //     Savers.push_back(std::thread(tagger, i));
    // }

    // std::cout << "help\r";

    
    run(10);
    
    // // Enqueue some data (for demonstration purposes)
    // for (int i = 1; i <= 10; ++i) {
    //     dataQueue.push(i);
    //     // Simulate data production time
    //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // }
    
    // Join Saver threads

    for (std::thread& producer : producers) {
        producer.join();
    }

    for (std::thread& saver : savers) {
        saver.join();
    }

    help.join();
}

int main() {

    start_threads();

    return 0;
}