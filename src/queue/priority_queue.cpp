#include "queue/priority_queue.hpp"
#include "queue/bounded_queue.hpp"
#include "types.hpp"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>

namespace dispatcher::queue {

PriorityQueue::QueuePtr PriorityQueue::makeQueue(QueueOptions opts) {
    QueuePtr result;
    if (opts.bounded) {
        if (!opts.capacity.has_value()) {
            throw std::invalid_argument("no way to create a bounded queue without capacity");
        }
        result = std::make_unique<BoundedQueue>(opts.capacity.value());
    } else {
        result = std::make_unique<UnboundedQueue>();
    }
    return result;
}

void PriorityQueue::push(TaskPriority priority, Task task) {
    if (!active_.load(std::memory_order_acquire)) {
        return;
    }
    if (static_cast<size_t>(priority) >= queues_.size()) {
        throw std::invalid_argument(std::format("unknown priority: {}", static_cast<int>(priority)));
    }
    queues_[static_cast<size_t>(priority)]->push(task);

    not_empty_.notify_one();
}

// block on pop until shutdown is called
// after that return std::nullopt on empty queue
std::optional<Task> PriorityQueue::pop() {
    std::unique_lock lock(mutex_);
    std::optional<Task> result;

    not_empty_.wait(lock, [&] {
        for (QueuePtr &q : queues_) {
            result = q->try_pop();
            if (result.has_value()) {
                return true;
            }
        }
        return !active_.load(std::memory_order_acquire);
    });

    return result;
}

void PriorityQueue::shutdown() {
    active_.store(false, std::memory_order_release);
    not_empty_.notify_all();
}

PriorityQueue::~PriorityQueue() { shutdown(); }

}  // namespace dispatcher::queue