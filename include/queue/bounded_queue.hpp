#pragma once
#include "queue/queue.hpp"
#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>

namespace dispatcher::queue {

class BoundedQueue : public IQueue {
    // здесь ваш код
public:
    explicit BoundedQueue(int capacity);
    ~BoundedQueue() override;

    void push(Task task) override;
    std::optional<Task> try_pop() override;

private:
    std::queue<Task> q_;
    std::mutex mutex_;
    std::condition_variable not_full_;

    size_t capacity_;
};

}  // namespace dispatcher::queue