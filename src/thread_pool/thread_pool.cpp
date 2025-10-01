#include "thread_pool/thread_pool.hpp"
#include "logger.hpp"
#include "queue/priority_queue.hpp"
#include <stop_token>
#include <thread>

namespace dispatcher::thread_pool {

using namespace queue;
using namespace std::chrono_literals;

ThreadPool::ThreadPool(std::shared_ptr<PriorityQueue> q, size_t num_threads) : queue_(q) { runThreads(num_threads); }
ThreadPool::~ThreadPool() {
    stop_source_.request_stop();
    queue_->shutdown();
}

void ThreadPool::runThreads(size_t num_threads) {
    workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back(&ThreadPool::worker, this, stop_source_.get_token());
    }
}
void ThreadPool::worker(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        std::optional<Task> task = queue_->pop();

        if (task.has_value()) {
            try {
                task.value()();
            } catch (const std::exception &e) {
                Logger::Get().Log("exception caught during task execution: " + std::string(e.what()));
            } catch (...) {
                Logger::Get().Log("unknown exception caught during task execution");
            }
        } else {
            std::this_thread::sleep_for(10ms);
        }
    }
}

}  // namespace dispatcher::thread_pool