//
// Created by fabian on 9/23/25.
//

#ifndef CHESS_SEARCHTHREADPOOL_H
#define CHESS_SEARCHTHREADPOOL_H


#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <future>
#include <atomic>

#include "../lib/surge/src/position.h"

struct SearchResult {
    int score;
    Move move;
};

class SearchThreadPool {
public:
    SearchThreadPool(size_t numThreads) {
        stop = false;
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this]() { this->worker(); });
        }
    }

    ~SearchThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        cv.notify_all();
        for (auto &t : workers) t.join();
    }

    void enqueue(std::packaged_task<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push(std::move(task));  // must move!
        }
        cv.notify_one();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::packaged_task<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool stop;

    void worker() {
        while (true) {
            std::packaged_task<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this]{ return stop || !tasks.empty(); });
                if (stop && tasks.empty()) return; // exit thread safely
                task = std::move(tasks.front());
                tasks.pop();
            }
            task(); // execute outside lock
        }
    }
};


#endif //CHESS_SEARCHTHREADPOOL_H