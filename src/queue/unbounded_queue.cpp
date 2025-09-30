#include "queue/unbounded_queue.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>

namespace dispatcher::queue {

void UnboundedQueue::push(Task task) {
    std::lock_guard lock(mutex_);
    q_.push(std::move(task));
}

std::optional<Task> UnboundedQueue::try_pop() {
    std::lock_guard lock(mutex_);

    if (q_.empty()) {
        return std::nullopt;
    }

    Task result = std::move(q_.front());
    q_.pop();
    return result;
}

}  // namespace dispatcher::queue