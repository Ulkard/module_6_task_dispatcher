#pragma once
#include "queue/bounded_queue.hpp"
#include "queue/queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <unordered_map>

namespace dispatcher::queue {

class PriorityQueue {
    // здесь ваш код
public:
    explicit PriorityQueue(QueueOptions q_opts_normal, QueueOptions q_opts_high);

    void push(TaskPriority priority, Task task);
    // block on pop until shutdown is called
    // after that return std::nullopt on empty queue
    std::optional<Task> pop();

    void shutdown();

    ~PriorityQueue();

private:
    std::unique_ptr<IQueue> q_normal_;
    std::unique_ptr<IQueue> q_high_;

    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::atomic<bool> active_ = true;

    std::unique_ptr<IQueue> makeQueue(QueueOptions opts);
};

}  // namespace dispatcher::queue