#pragma once
#include "queue/bounded_queue.hpp"
#include "queue/queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <unordered_map>
#include <vector>

namespace dispatcher::queue {

class PriorityQueue {
public:
    template <std::same_as<QueueOptions>... QueueOptionsTs>
    PriorityQueue(QueueOptionsTs... args) {
        queues_.reserve(sizeof...(args));
        (queues_.push_back(makeQueue(args)), ...);
    }

    void push(TaskPriority priority, Task task);
    // block on pop until shutdown is called
    // after that return std::nullopt on empty queue
    std::optional<Task> pop();

    void shutdown();

    ~PriorityQueue();

private:
    using QueuePtr = std::unique_ptr<IQueue>;
    std::vector<QueuePtr> queues_;

    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::atomic<bool> active_ = true;

    QueuePtr makeQueue(QueueOptions opts);
};

}  // namespace dispatcher::queue