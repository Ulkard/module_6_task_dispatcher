#pragma once
#include "queue/queue.hpp"
#include <mutex>
#include <queue>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
    // здесь ваш код
public:
    UnboundedQueue() = default;
    ~UnboundedQueue() override = default;

    void push(Task task) override;
    std::optional<Task> try_pop() override;

private:
    std::queue<Task> q_;
    std::mutex mutex_;
};

}  // namespace dispatcher::queue