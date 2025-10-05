#include "queue/priority_queue.hpp"
#include "queue/queue.hpp"
#include "types.hpp"
#include <gtest/gtest.h>
#include <optional>

using namespace dispatcher::queue;
using namespace std::chrono_literals;
using dispatcher::TaskPriority;

PriorityQueue makeQueue(int cap_normal = 0, int cap_high = 0) {
    QueueOptions opts_normal{cap_normal != 0};
    if (cap_normal != 0) {
        opts_normal.capacity = cap_normal;
    }
    QueueOptions opts_high{cap_high != 0};
    if (cap_high != 0) {
        opts_high.capacity = cap_high;
    }
    return PriorityQueue(opts_normal, opts_high);
}

template <typename Predicate>
bool waitFor(Predicate pred, std::chrono::milliseconds timeout = 100ms) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

TEST(PriorityQueue, PushDifferentPriorities) {
    PriorityQueue queue = makeQueue();

    std::atomic<int> normal_executed{0};
    std::atomic<int> high_executed{0};

    // Push tasks with different priorities
    queue.push(TaskPriority::Normal, [&normal_executed]() { normal_executed.fetch_add(1); });
    queue.push(TaskPriority::High, [&high_executed]() { high_executed.fetch_add(1); });
    queue.push(TaskPriority::Normal, [&normal_executed]() { normal_executed.fetch_add(2); });
    queue.push(TaskPriority::High, [&high_executed]() { high_executed.fetch_add(2); });

    // Pop and execute all tasks
    std::vector<Task> tasks;
    for (int i = 0; i < 4; ++i) {
        auto task = queue.pop();
        tasks.push_back(std::move(*task));
    }

    // Execute tasks
    for (auto &task : tasks) {
        task();
    }

    EXPECT_EQ(normal_executed.load(), 3);
    EXPECT_EQ(high_executed.load(), 3);
}

TEST(PriorityQueue, PriorityOrdering) {
    PriorityQueue queue = makeQueue();

    std::vector<TaskPriority> execution_order;
    std::mutex order_mutex;

    // Push mixed priority tasks
    queue.push(TaskPriority::Normal, [&]() {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(TaskPriority::Normal);
    });
    queue.push(TaskPriority::High, [&]() {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(TaskPriority::High);
    });
    queue.push(TaskPriority::Normal, [&]() {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(TaskPriority::Normal);
    });
    queue.push(TaskPriority::High, [&]() {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(TaskPriority::High);
    });

    // Pop and execute all tasks
    std::vector<Task> tasks;
    for (int i = 0; i < 4; ++i) {
        auto task = queue.pop();
        EXPECT_TRUE(task.has_value());
        tasks.push_back(std::move(*task));
    }

    // Execute tasks in pop order
    for (auto &task : tasks) {
        task();
    }

    // High priority tasks should execute first
    EXPECT_EQ(execution_order.size(), 4u);
    EXPECT_EQ(execution_order[0], TaskPriority::High);
    EXPECT_EQ(execution_order[1], TaskPriority::High);
    EXPECT_EQ(execution_order[2], TaskPriority::Normal);
    EXPECT_EQ(execution_order[3], TaskPriority::Normal);
}

TEST(PriorityQueue, PopBlocksWhenEmpty) {
    PriorityQueue queue = makeQueue();

    std::atomic<bool> pop_completed{false};
    std::atomic<bool> pop_started{false};

    std::thread popper([&]() {
        pop_started.store(true);
        auto task = queue.pop();
        pop_completed.store(true);
        if (task.has_value()) {
            (*task)();
        }
    });

    // Wait for popper to start and block
    waitFor([&]() { return pop_started.load(); });
    std::this_thread::sleep_for(50ms);

    // Pop shouldn't have completed yet because queue is empty
    EXPECT_FALSE(pop_completed.load());

    // Push a task to unblock pop
    std::atomic<bool> task_executed{false};
    queue.push(TaskPriority::Normal, [&task_executed]() { task_executed.store(true); });

    // Wait for pop to complete
    popper.join();
    EXPECT_TRUE(pop_completed.load());
    EXPECT_TRUE(task_executed.load());
}

TEST(PriorityQueue, ShutdownUnblocksPop) {
    PriorityQueue queue = makeQueue();

    std::atomic<bool> pop_completed{false};
    std::atomic<bool> got_task{true};

    std::thread popper([&]() {
        auto task = queue.pop();
        pop_completed.store(true);
        got_task.store(task.has_value());
    });

    // Wait for popper to block
    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(pop_completed.load());

    // Shutdown should unblock pop
    queue.shutdown();

    // Pop should complete with nullopt
    EXPECT_TRUE(waitFor([&]() { return pop_completed.load(); }));
    popper.join();
    EXPECT_FALSE(got_task.load());
}

TEST(PriorityQueue, DestructorCallsShutdown) {
    std::atomic<bool> pop_completed{false};

    std::thread popper([&pop_completed]() {
        auto local_queue = std::make_unique<PriorityQueue>(QueueOptions{}, QueueOptions{});

        std::thread inner_popper([&local_queue, &pop_completed]() {
            auto task = local_queue->pop();
            pop_completed.store(true);
        });

        // Destroy queue while pop is blocking
        std::this_thread::sleep_for(50ms);
        local_queue.reset();

        inner_popper.join();
    });

    popper.join();
    EXPECT_TRUE(pop_completed.load());
}

TEST(PriorityQueue, BoundedQueues) {
    PriorityQueue queue = makeQueue(3, 5);

    std::atomic<int> normal_executed{0};
    std::atomic<int> high_executed{0};

    // Fill both queues to capacity
    for (int i = 0; i < 5; ++i) {
        queue.push(TaskPriority::Normal, [&normal_executed]() { normal_executed.fetch_add(1); });
    }

    for (int i = 0; i < 3; ++i) {
        queue.push(TaskPriority::High, [&high_executed]() { high_executed.fetch_add(1); });
    }

    // Execute all tasks
    for (int i = 0; i < 8; ++i) {
        auto task = queue.pop();
        EXPECT_TRUE(task.has_value());
        (*task)();
    }

    EXPECT_EQ(normal_executed.load(), 5);
    EXPECT_EQ(high_executed.load(), 3);
}

// Test mixed bounded/unbounded queues
TEST(PriorityQueue, MixedQueueTypes) {
    PriorityQueue queue = makeQueue(0, 10);

    std::atomic<int> normal_executed{0};
    std::atomic<int> high_executed{0};

    // Push many high priority tasks (unbounded)
    for (int i = 0; i < 100; ++i) {
        queue.push(TaskPriority::High, [&high_executed]() { high_executed.fetch_add(1); });
    }

    // Push normal priority tasks (bounded)
    for (int i = 0; i < 10; ++i) {
        queue.push(TaskPriority::Normal, [&normal_executed]() { normal_executed.fetch_add(1); });
    }

    // Execute all tasks
    int executed_count = 0;
    while (executed_count < 110) {
        auto task = queue.pop();
        if (task.has_value()) {
            (*task)();
            executed_count++;
        }
    }

    EXPECT_EQ(normal_executed.load(), 10);
    EXPECT_EQ(high_executed.load(), 100);
}

// Test that high priority always takes precedence
TEST(PriorityQueue, HighPriorityPrecedence) {
    PriorityQueue queue = makeQueue();

    const int num_normal = 100;
    const int num_high = 50;

    std::vector<int> execution_order;
    std::mutex order_mutex;

    // Fill with normal priority tasks
    for (int i = 0; i < num_normal; ++i) {
        queue.push(TaskPriority::Normal, [&, i]() {
            std::lock_guard lock(order_mutex);
            execution_order.push_back(-i);  // Negative for normal
        });
    }

    // Add high priority tasks
    for (int i = 0; i < num_high; ++i) {
        queue.push(TaskPriority::High, [&, i]() {
            std::lock_guard lock(order_mutex);
            execution_order.push_back(i + 1000);  // Positive + offset for high
        });
    }

    // Execute all tasks
    for (int i = 0; i < num_normal + num_high; ++i) {
        auto task = queue.pop();
        EXPECT_TRUE(task.has_value());
        (*task)();
    }

    // First num_high tasks should be high priority
    for (int i = 0; i < num_high; ++i) {
        EXPECT_GE(execution_order[i], 1000);
    }
}

TEST(PriorityQueue, InvalidPriority) {
    PriorityQueue queue = makeQueue();

    // This should throw due to invalid priority
    EXPECT_THROW({ queue.push(static_cast<TaskPriority>(999), []() {}); }, std::invalid_argument);
}

TEST(PriorityQueue, ShutdownWithActiveProducers) {
    PriorityQueue queue = makeQueue();

    std::atomic<int> tasks_pushed{0};
    std::atomic<bool> producer_stopped{false};

    // Start producer that pushes many tasks
    std::thread producer([&]() {
        for (int i = 0; i < 10000; ++i) {
            if (!producer_stopped.load()) {
                queue.push(TaskPriority::Normal, []() {});
                tasks_pushed.fetch_add(1);
            }
        }
    });

    // Start consumer
    std::thread consumer([&]() {
        int consumed = 0;
        while (consumed < 100) {
            auto task = queue.pop();
            if (task.has_value()) {
                consumed++;
            }
        }
    });

    // Let them run for a bit
    std::this_thread::sleep_for(10ms);

    // Shutdown should stop the queue
    queue.shutdown();
    producer_stopped.store(true);

    producer.join();
    consumer.join();

    EXPECT_GT(tasks_pushed.load(), 0);
}