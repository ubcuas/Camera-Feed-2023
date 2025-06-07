// Copyright 2024 UBC Uncrewed Aircraft Systems

#ifndef SRC_TSQUEUE_HPP_
#define SRC_TSQUEUE_HPP_

#include <utility>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>

struct AbortedPopException {};

// Thread-safe queue
// https://www.geeksforgeeks.org/implement-thread-safe-queue-in-c/
template <typename T>
class TSQueue {
 private:
  std::queue<T> m_queue;

  std::mutex m_mutex;

  std::condition_variable m_cond;

  mutable std::atomic<bool> abort_flag = false;

 public:
  // Pushes an element to the queue
  void push(T item) {
    // Acquire lock
    std::lock_guard<std::mutex> lock(m_mutex);

    // Add item and transfers ownership to internal queue
    m_queue.push(std::move(item));

    // Notify one thread that
    // is waiting
    m_cond.notify_one();
  }

  // Pops an element off the queue
  T pop() {
    // acquire lock
    std::unique_lock<std::mutex> lock(m_mutex);

    // wait until queue is not empty
    m_cond.wait(lock, [this]() { return !m_queue.empty() || abort_flag; });

    if (abort_flag) throw AbortedPopException{};

    // retrieve item and transfers ownership to here
    T item = std::move(m_queue.front());
    m_queue.pop();

    // return item
    return item;
  }

  // Releases blocked threads
  void abort() {
    abort_flag = true;
    m_cond.notify_all();
  }
};

#endif  // SRC_TSQUEUE_HPP_
