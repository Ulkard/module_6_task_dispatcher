#pragma once

#include <memory>

#include "queue/priority_queue.hpp"
#include "queue/queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"

namespace dispatcher {

class TaskDispatcher {
public:
    TaskDispatcher(size_t thread_count, queue::QueueOptions q_opts_normal = {.bounded = false},
                   queue::QueueOptions q_opts_high = {.bounded = true, .capacity = 1000});

    void schedule(TaskPriority priority, queue::Task task);

private:
    std::shared_ptr<queue::PriorityQueue> queue_;
    thread_pool::ThreadPool thread_pool_;
};

}  // namespace dispatcher