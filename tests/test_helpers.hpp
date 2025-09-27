#pragma once

#include "thread"

template <typename T>
class Producer {
public:
    Producer(T &queue, size_t total) : queue_(queue), remaining_(total) {
        thread_ = std::jthread([this] { Run(); });
    }

    void Wait() {
        if (thread_.joinable())
            thread_.join();
    }

    size_t Size() const { return remaining_; }

private:
    void Run() {
        for (auto i = remaining_.load(); i > 0; --remaining_) {
            queue_.push(i);
        }
    }

    T &queue_;
    std::atomic<size_t> remaining_{0};
    std::jthread thread_;
};

template <typename T>
class CountedConsumer {
public:
    explicit CountedConsumer(T &queue) : queue_(queue) {
        // thread_ = std::jthread([this] { Run(); });
    }

    void Wait() {
        if (thread_.joinable())
            thread_.join();
    }

    size_t Size() const { return count_; }

private:
    void Run() {
        while (queue_.CanConsume()) {
            auto item = queue_.try_pop();
            if (item.has_value()) {
                ++count_;
            }
        }
    }

    T &queue_;
    std::atomic<size_t> count_{0};
    std::thread thread_;
};
