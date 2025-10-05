#pragma once

#include "queue/priority_queue.hpp"
#include <stop_token>
#include <thread>

namespace dispatcher::thread_pool {

class ThreadPool {
public:
    ThreadPool(std::shared_ptr<queue::PriorityQueue> q, size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

private:
    void runThreads(size_t num_threads);
    void worker(std::stop_token stoken);

    std::stop_source stop_source_;
    std::shared_ptr<queue::PriorityQueue> queue_;
    std::vector<std::jthread> workers;
};

}  // namespace dispatcher::thread_pool
