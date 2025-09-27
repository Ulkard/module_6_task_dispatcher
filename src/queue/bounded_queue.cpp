#include "queue/bounded_queue.hpp"

namespace dispatcher::queue {

BoundedQueue::BoundedQueue(int capacity) : capacity_(capacity) {}

void BoundedQueue::push(std::function<void()> task) {
    std::unique_lock lock(mutex_);
    not_full_.wait(lock, [this] { return q_.size() < capacity_ || !active_; });

    if (!active_)
        return;

    q_.push(std::move(task));
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    std::unique_lock lock(mutex_);

    if (q_.empty())
        return std::nullopt;

    std::function<void()> result = std::move(q_.front());
    q_.pop();

    not_full_.notify_one();
    return result;
}

BoundedQueue::~BoundedQueue() {}

}  // namespace dispatcher::queue
