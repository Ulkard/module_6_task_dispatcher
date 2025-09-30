#pragma once
#include <functional>
#include <optional>

namespace dispatcher::queue {

using Task = std::function<void()>;

struct QueueOptions {
    bool bounded;
    std::optional<int> capacity;
};

class IQueue {
public:
    virtual ~IQueue() = default;
    virtual void push(Task task) = 0;
    virtual std::optional<Task> try_pop() = 0;
};

}  // namespace dispatcher::queue