#include "queue/priority_queue.hpp"
#include "queue/bounded_queue.hpp"
#include "types.hpp"
#include <atomic>
#include <cassert>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>

namespace dispatcher::queue {

PriorityQueue::PriorityQueue(QueueOptions q_opts_normal, QueueOptions q_opts_high)
    : q_normal_(makeQueue(q_opts_normal)), q_high_(makeQueue(q_opts_high)) {}

std::unique_ptr<IQueue> PriorityQueue::makeQueue(QueueOptions opts) {
    std::unique_ptr<IQueue> result;
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
    switch (priority) {
    case TaskPriority::High:
        q_high_->push(std::move(task));
        break;
    case TaskPriority::Normal:
        q_normal_->push(std::move(task));
        break;
    default:
        throw std::invalid_argument(std::format("unknown priority: {}", static_cast<int>(priority)));
    }
    not_empty_.notify_one();
}

// block on pop until shutdown is called
// after that return std::nullopt on empty queue
std::optional<Task> PriorityQueue::pop() {
    std::unique_lock lock(mutex_);
    std::optional<Task> result;

    not_empty_.wait(lock, [&] {
        return (result = q_high_->try_pop()).has_value() || (result = q_normal_->try_pop()).has_value() ||
               !active_.load(std::memory_order_acquire);
    });

    return result;
}

void PriorityQueue::shutdown() {
    active_.store(false, std::memory_order_release);
    not_empty_.notify_all();
}

PriorityQueue::~PriorityQueue() { shutdown(); }

}  // namespace dispatcher::queue