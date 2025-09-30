#include "queue/bounded_queue.hpp" / Include your header file
#include "test_helpers.hpp"

using namespace dispatcher::queue;
using namespace std::chrono_literals;

TEST(BoundedQueue, SingleThreadedPushPop) {
    BoundedQueue queue(2);
    testSingleThreadPushPop(queue);
}

TEST(BoundedQueue, TryPopEmptyQueue) {
    BoundedQueue queue(3);
    testPopEmptyQueue(queue);
}

TEST(BoundedQueue, MultipleProducers) {
    BoundedQueue queue(100);
    const int num_tasks = 100;
    const int num_producers = 4;

    testMultipleProducers(queue, num_tasks, num_producers);
}

TEST(BoundedQueue, MultipleConsumers) {
    BoundedQueue queue(500);
    const int num_tasks = 500;
    const int num_consumers = 5;

    testMultipleConsumers(queue, num_tasks, num_consumers);
}

TEST(BoundedQueue, ProducerConsumer) {
    BoundedQueue queue(10);
    const int num_tasks_per_producer = 250;
    const int num_producers = 2;
    const int num_consumers = 2;

    testProducerConsumer(queue, num_tasks_per_producer, num_producers, num_consumers);
}

TEST(BoundedQueue, FIFOOrder) {
    BoundedQueue queue(5);
    int num_tasks = 5;

    testFIFOOrder(queue, num_tasks);
}

TEST(BoundedQueue, PushBlocksWhenFull) {
    BoundedQueue queue(2);

    std::atomic<int> counter{0};

    // Fill the queue
    queue.push([&counter]() { counter.fetch_add(1); });
    queue.push([&counter]() { counter.fetch_add(1); });

    std::atomic<bool> push_completed{false};
    std::atomic<bool> push_started{false};

    std::thread pusher([&]() {
        push_started.store(true);
        queue.push([&counter]() { counter.fetch_add(1); });
        push_completed.store(true);
    });

    // Wait for pusher thread to start and try to push
    while (!push_started.load()) {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(50ms);

    // Push shouldn't have completed yet because queue is full
    EXPECT_FALSE(push_completed.load());

    // Make space by popping and executing
    auto task = queue.try_pop();
    EXPECT_TRUE(task.has_value());
    (*task)();

    // Now push should complete
    pusher.join();
    EXPECT_TRUE(push_completed.load());
}