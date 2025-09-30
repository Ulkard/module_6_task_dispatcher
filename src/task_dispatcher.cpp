#include "task_dispatcher.hpp"
#include "queue/priority_queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include <memory>

namespace dispatcher {

using namespace queue;

TaskDispatcher::TaskDispatcher(size_t thread_count, queue::QueueOptions q_opts_normal, queue::QueueOptions q_opts_high)
    : queue_(std::make_shared<PriorityQueue>(q_opts_normal, q_opts_normal)), thread_pool_(queue_, thread_count) {}

void TaskDispatcher::schedule(TaskPriority priority, Task task) { queue_->push(priority, std::move(task)); }

}  // namespace dispatcher