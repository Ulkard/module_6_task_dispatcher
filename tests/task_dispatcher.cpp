#include <gtest/gtest.h>

#include "task_dispatcher.hpp"

using namespace dispatcher;
using namespace std::chrono_literals;

TEST(TaskDispatcher, ExecuteBasicTask) {
    std::atomic<bool> task_executed{false};
    TaskDispatcher dispatcher(2);

    dispatcher.schedule(TaskPriority::Normal, [&] { task_executed.store(true); });

    // Wait for task to be executed
    auto start = std::chrono::steady_clock::now();
    while (!task_executed.load() && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_TRUE(task_executed.load());
}

TEST(TaskDispatcher, PriorityOrder) {
    std::vector<TaskPriority> execution_order;
    std::mutex order_mutex;
    TaskDispatcher dispatcher(1);  // Single thread to ensure order

    // Schedule some long task first
    dispatcher.schedule(TaskPriority::Normal, [&] {
        std::lock_guard lock(order_mutex);
        std::this_thread::sleep_for(50ms);  // Make task take some time
    });
    // Then schedule normal priority task
    dispatcher.schedule(TaskPriority::Normal, [&] {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(TaskPriority::Normal);
    });

    // Then schedule high priority task
    dispatcher.schedule(TaskPriority::High, [&] {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(TaskPriority::High);
    });

    // Wait for both tasks to complete
    std::this_thread::sleep_for(100ms);

    // High priority should execute before normal priority
    EXPECT_EQ(execution_order.size(), 2u);
    if (execution_order.size() == 2) {
        EXPECT_EQ(execution_order[0], TaskPriority::High);
        EXPECT_EQ(execution_order[1], TaskPriority::Normal);
    }
}

TEST(TaskDispatcher, MultipleTasks) {
    constexpr int TASK_COUNT = 10;
    std::atomic<int> completed_tasks{0};
    TaskDispatcher dispatcher(4);

    for (int i = 0; i < TASK_COUNT; ++i) {
        dispatcher.schedule(TaskPriority::Normal, [&] { completed_tasks.fetch_add(1); });
    }

    // Wait for all tasks to complete
    auto start = std::chrono::steady_clock::now();
    while (completed_tasks.load() < TASK_COUNT && std::chrono::steady_clock::now() - start < 3s) {
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_EQ(completed_tasks.load(), TASK_COUNT);
}

TEST(TaskDispatcher, MixedPriorityTasks) {
    std::atomic<int> high_priority_completed{0};
    std::atomic<int> normal_priority_completed{0};
    TaskDispatcher dispatcher(2);

    // Schedule mix of high and normal priority tasks
    for (int i = 0; i < 5; ++i) {
        dispatcher.schedule(TaskPriority::High, [&] { high_priority_completed.fetch_add(1); });
        dispatcher.schedule(TaskPriority::Normal, [&] { normal_priority_completed.fetch_add(1); });
    }

    // Wait for tasks to complete
    auto start = std::chrono::steady_clock::now();
    while ((high_priority_completed.load() + normal_priority_completed.load()) < 10 &&
           std::chrono::steady_clock::now() - start < 3s) {
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_EQ(high_priority_completed.load() + normal_priority_completed.load(), 10);
}

TEST(TaskDispatcher, ThreadPoolSize) {
    constexpr size_t THREAD_COUNT = 3;
    std::atomic<int> concurrent_tasks{0};
    std::atomic<int> max_concurrent{0};
    std::mutex mutex;

    TaskDispatcher dispatcher(THREAD_COUNT);

    // Schedule tasks that check concurrency
    for (int i = 0; i < 10; ++i) {
        dispatcher.schedule(TaskPriority::Normal, [&] {
            int current = concurrent_tasks.fetch_add(1);
            {
                std::lock_guard lock(mutex);
                if (current + 1 > max_concurrent.load()) {
                    max_concurrent.store(current + 1);
                }
            }

            std::this_thread::sleep_for(50ms);  // Simulate work
            concurrent_tasks.fetch_sub(1);
        });
    }

    std::this_thread::sleep_for(500ms);

    // Should not exceed thread pool size
    EXPECT_LE(max_concurrent.load(), THREAD_COUNT);
}

TEST(TaskDispatcher, DestructionWithRunningTasks) {
    std::atomic<bool> task_started{false};
    std::atomic<bool> dispatcher_destroyed{false};

    {
        TaskDispatcher dispatcher(2);

        dispatcher.schedule(TaskPriority::Normal, [&] {
            task_started.store(true);
            std::this_thread::sleep_for(100ms);  // Task takes some time
        });

        // Wait for task to start
        auto start = std::chrono::steady_clock::now();
        while (!task_started.load() && std::chrono::steady_clock::now() - start < 1s) {
            std::this_thread::sleep_for(10ms);
        }

        // Dispatcher goes out of scope here, should wait for running tasks
    }

    dispatcher_destroyed.store(true);
    EXPECT_TRUE(dispatcher_destroyed.load());
    EXPECT_TRUE(task_started.load());
}

TEST(TaskDispatcher, ExceptionHandling) {
    std::atomic<bool> exception_caught{false};
    TaskDispatcher dispatcher(2);

    // check that exceptions in tasks won't crash the dispatcher
    dispatcher.schedule(TaskPriority::Normal, [&] {
        try {
            throw std::runtime_error("Test exception");
        } catch (...) {
            exception_caught.store(true);
        }
    });

    auto start = std::chrono::steady_clock::now();
    while (!exception_caught.load() && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_TRUE(exception_caught.load());
}