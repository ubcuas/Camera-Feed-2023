#include <thread>
#include <vector>

#include "TSQueue.h"

bool stopFlag = false;

TSQueue<int> ImageQueue;
TSQueue<int> TagQueue;

int image_num = 0;

void run(int seconds) {
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
    TagQueue.push(r);
    // std::cout << "Popped " << r << "\n";
  }
}

void printer() {
  while (!stopFlag) {
    std::this_thread::sleep_for(std::chrono::seconds());
    std::cout << "ImageQueue: " << ImageQueue.size()
              << ", TagQueue: " << TagQueue.size() << "\r";
  }
  std::cout << "\n";
}

void tagger() {
  while (!stopFlag) {
    int r = TagQueue.pop();
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

void start_threads() {
  const int numProducers = 1;
  const int numSavers = 3;
  const int numTaggers = 1;

  std::vector<std::thread> producers;
  std::vector<std::thread> savers;
  std::vector<std::thread> taggers;

  for (int i = 0; i < numProducers; i++) {
    savers.push_back(std::thread(producer));
  }

  for (int i = 0; i < numSavers; i++) {
    savers.push_back(std::thread(saver));
  }

  for (int i = 0; i < numTaggers; ++i) {
    taggers.push_back(std::thread(tagger));
  }

  std::thread help(printer);

  // std::cout << "help\r";

  run(20);

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

  for (std::thread& tagger : taggers) {
    tagger.join();
  }

  help.join();
}

int main() {
  start_threads();

  return 0;
}