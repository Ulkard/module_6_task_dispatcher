#include "queue/unbounded_queue.hpp"
#include "test_helpers.hpp"

using namespace dispatcher::queue;

TEST(UnboundedQueue, SingleThreadedPushPop) {
    UnboundedQueue queue;
    testSingleThreadPushPop(queue);
}

TEST(UnboundedQueue, TryPopEmptyQueue) {
    UnboundedQueue queue;
    testPopEmptyQueue(queue);
}

// Test multiple producers - unbounded queue should never block on push
TEST(UnboundedQueue, MultipleProducers) {
    UnboundedQueue queue;
    const int num_tasks = 1000;
    const int num_producers = 4;

    testMultipleProducers(queue, num_tasks, num_producers);
}

// Test multiple consumers
TEST(UnboundedQueue, MultipleConsumers) {
    UnboundedQueue queue;
    const int num_tasks = 500;
    const int num_consumers = 5;

    testMultipleConsumers(queue, num_tasks, num_consumers);
}

// Test producer-consumer scenario
TEST(UnboundedQueue, ProducerConsumer) {
    UnboundedQueue queue;
    const int num_tasks_per_producer = 250;
    const int num_producers = 4;
    const int num_consumers = 4;

    testProducerConsumer(queue, num_tasks_per_producer, num_producers, num_consumers);
}

TEST(UnboundedQueue, FIFOOrder) {
    UnboundedQueue queue;
    int num_tasks = 5;

    testFIFOOrder(queue, num_tasks);
}

TEST(UnboundedQueue, PushNeverBlocks) {
    UnboundedQueue queue;

    std::atomic<int> counter{0};
    const int large_number = 10000;

    // Push a large number of tasks - should never block
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < large_number; ++i) {
        queue.push([&counter]() { counter.fetch_add(1); });
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Pushing should be very fast (no blocking)
    EXPECT_LT(duration.count(), 1000);  // Should complete in less than 1 second

    // Verify all tasks were pushed by executing them
    int executed_count = 0;
    for (int i = 0; i < large_number; ++i) {
        auto task = queue.try_pop();
        if (task.has_value()) {
            (*task)();
            executed_count++;
        }
    }

    EXPECT_EQ(executed_count, large_number);
    EXPECT_EQ(counter.load(), large_number);
}

// Test memory usage with large number of tasks (unbounded growth)
TEST(UnboundedQueue, UnboundedGrowth) {
    UnboundedQueue queue;
    const int large_number = 100000;

    // Push a very large number of tasks - should not fail
    for (int i = 0; i < large_number; ++i) {
        queue.push([]() { /* empty task */ });
    }

    // Verify we can pop all tasks
    int popped_count = 0;
    for (int i = 0; i < large_number; ++i) {
        auto task = queue.try_pop();
        if (task.has_value()) {
            popped_count++;
        }
    }

    EXPECT_EQ(popped_count, large_number);

    // Queue should be empty now
    auto final_task = queue.try_pop();
    EXPECT_FALSE(final_task.has_value());
}