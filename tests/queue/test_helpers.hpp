#pragma once

#include "queue/queue.hpp"
#include <atomic>
#include <gtest/gtest.h>

inline void testSingleThreadPushPop(dispatcher::queue::IQueue &queue) {
    std::atomic<int> counter{0};

    // Push tasks that increment counter
    queue.push([&counter]() { counter.fetch_add(1); });
    queue.push([&counter]() { counter.fetch_add(2); });

    // Execute popped tasks
    auto popped1 = queue.try_pop();
    auto popped2 = queue.try_pop();

    EXPECT_TRUE(popped1.has_value());
    EXPECT_TRUE(popped2.has_value());

    (*popped1)();  // Execute first task
    (*popped2)();  // Execute second task

    EXPECT_EQ(counter.load(), 3);
}

inline void testPopEmptyQueue(dispatcher::queue::IQueue &queue) {
    auto result = queue.try_pop();
    EXPECT_FALSE(result.has_value());
}

inline void testMultipleProducers(dispatcher::queue::IQueue &queue, int num_tasks, int num_producers) {
    std::vector<std::thread> producers;
    std::atomic<int> tasks_executed{0};
    std::atomic<int> tasks_pushed{0};

    // Create producers
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < num_tasks / num_producers; ++j) {
                queue.push([&tasks_executed]() { tasks_executed.fetch_add(1); });
                tasks_pushed.fetch_add(1);
            }
        });
    }

    // Wait for all producers
    for (auto &producer : producers) {
        producer.join();
    }

    // Verify all tasks were pushed
    EXPECT_EQ(tasks_pushed.load(), num_tasks);

    // Execute all tasks and count them
    int executed_count = 0;
    for (int i = 0; i < num_tasks; ++i) {
        auto task = queue.try_pop();
        if (task.has_value()) {
            (*task)();  // Execute the task
            executed_count++;
        }
    }

    EXPECT_EQ(executed_count, num_tasks);
    EXPECT_EQ(tasks_executed.load(), num_tasks);
}

inline void testMultipleConsumers(dispatcher::queue::IQueue &queue, int num_tasks, int num_consumers) {
    std::atomic<int> execution_count{0};

    // Pre-fill the queue with tasks
    for (int i = 0; i < num_tasks; ++i) {
        queue.push([&execution_count]() { execution_count.fetch_add(1); });
    }

    std::vector<std::thread> consumers;
    std::atomic<int> total_consumed{0};

    // Create consumers
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            int local_consumed = 0;
            while (local_consumed < num_tasks / num_consumers) {
                auto task = queue.try_pop();
                if (task.has_value()) {
                    (*task)();  // Execute task
                    total_consumed.fetch_add(1);
                    local_consumed++;
                }
            }
        });
    }

    // Wait for all consumers
    for (auto &consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(total_consumed.load(), num_tasks);
    EXPECT_EQ(execution_count.load(), num_tasks);
}

inline void testProducerConsumer(dispatcher::queue::IQueue &queue, int tasks_per_producer, int num_producers,
                                 int num_consumers) {
    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    std::atomic<int> executed_count{0};

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Start producers
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < tasks_per_producer; ++j) {
                queue.push([&executed_count]() { executed_count.fetch_add(1); });
                produced_count.fetch_add(1);
            }
        });
    }

    // Start consumers
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            int local_consumed = 0;
            while (local_consumed < tasks_per_producer * num_producers / num_consumers) {
                auto task = queue.try_pop();
                if (task.has_value()) {
                    (*task)();  // Execute the task
                    consumed_count.fetch_add(1);
                    local_consumed++;
                }
            }
        });
    }

    // Wait for producers
    for (auto &producer : producers) {
        producer.join();
    }

    // Wait for consumers
    for (auto &consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(produced_count.load(), tasks_per_producer * num_producers);
    EXPECT_EQ(consumed_count.load(), tasks_per_producer * num_producers);
    EXPECT_EQ(executed_count.load(), tasks_per_producer * num_producers);
}

inline void testFIFOOrder(dispatcher::queue::IQueue &queue, int num_tasks) {
    std::vector<int> execution_order;
    std::mutex order_mutex;

    // Push tasks that record their execution order
    for (int i = 0; i < num_tasks; ++i) {
        queue.push([&, i]() {
            std::lock_guard lock(order_mutex);
            execution_order.push_back(i);
        });
    }

    // Execute all tasks
    for (int i = 0; i < num_tasks; ++i) {
        auto task = queue.try_pop();
        EXPECT_TRUE(task.has_value());
        (*task)();
    }

    // Verify FIFO order
    EXPECT_EQ(execution_order.size(), 5u);
    for (int i = 0; i < num_tasks; ++i) {
        EXPECT_EQ(execution_order[i], i);
    }
}