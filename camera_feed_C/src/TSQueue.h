#ifndef TSQUEUE_H
#define TSQUEUE_H

#include <iostream> 
#include <mutex> 
#include <queue> 
#include <atomic> 
#include <condition_variable> 

struct AbortedPopException {};


// Thread-safe queue https://www.geeksforgeeks.org/implement-thread-safe-queue-in-c/
template <typename T> 
class TSQueue { 
private: 
    // Underlying queue 
    std::queue<T> m_queue; 
  
    // mutex for thread synchronization 
    std::mutex m_mutex; 
  
    // Condition variable for signaling 
    std::condition_variable m_cond; 

    mutable std::atomic<bool> abort_flag = false;

  
public: 
    // Pushes an element to the queue 
    void push(T item) 
    { 
        // Acquire lock 
        std::unique_lock<std::mutex> lock(m_mutex); 
  
        // Add item 
        m_queue.push(item); 
  
        // Notify one thread that 
        // is waiting 
        m_cond.notify_one(); 
    } 
  
    // Pops an element off the queue 
    T pop() 
    { 
  
        // acquire lock 
        std::unique_lock<std::mutex> lock(m_mutex); 
  
        // wait until queue is not empty 
        m_cond.wait(lock, 
                    [this]() { return !m_queue.empty() || abort_flag; }); 

        if (abort_flag)
            throw AbortedPopException{};
  
        // retrieve item 
        T item = m_queue.front(); 
        m_queue.pop(); 
  
        // return item 
        return item; 
    } 

    void abort() {
        abort_flag = true;
        m_cond.notify_all(); 
    }

}; 


#endif
