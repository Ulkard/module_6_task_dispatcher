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

    void push(std::function<void()> task) override;
    std::optional<std::function<void()>> try_pop() override;

private:
    std::queue<std::function<void()>, std::list<std::function<void()>>> q_;
    std::mutex mutex_;
    std::condition_variable not_full_;

    size_t capacity_;
    bool active_ = true;
};

}  // namespace dispatcher::queue