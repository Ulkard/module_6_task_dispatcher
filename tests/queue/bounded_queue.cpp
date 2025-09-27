#include <gtest/gtest.h>
#include <ostream>

#include "../test_helpers.hpp"
#include "print"
#include "queue/bounded_queue.hpp"

using namespace std::chrono_literals;
using namespace dispatcher::queue;

TEST(BoundedQueue, singleWriter) {
    /* SyncedQueue<int> queue(100);

    Producer tx{queue, 10'000};  // Создаём одного производителя, который добавит в очередь 10'000 элементов
    CountedConsumer rx{queue};  // Создаём одного потребителя, который будет обрабатывать добавленные в очередь элементы

    // Ждём, пока производитель добавит все задачи в очередь, и останавливаем обработку новых задач
    tx.Wait();
    queue.StopAcceptPushes();

    // Ждём, пока CountedConsumer обработает все задачи
    rx.Wait();

    // Проверяем, что Producer добавил все свои задачи в очередь
    if (tx.Size() != 0) {
        std::println("Ошибка! Производитель отправил не все задачи на исполнение");
    }

    // Проверяем, что CountedConsumer обработал все задачи
    if (rx.Size() != 10'000) {
        std::println("Ошибка! Потребитель обработал не все задачи");
    } */
}

TEST(BoundedQueue, multipleWriter) {
    BoundedQueue q(10);

    //
    // Создаём три потока, добавляющие данные в очередь
    //
    std::jthread t1{[&q] {
        for (auto _ : std::views::iota(0, 1'000)) {
            std::this_thread::sleep_for(1ms);
            q.push({});
        }
    }};
    std::jthread t2{[&q] {
        for (auto _ : std::views::iota(0, 1'000)) {
            std::this_thread::sleep_for(1ms);
            q.push({});
        }
    }};
    std::jthread t3{[&q] {
        for (auto _ : std::views::iota(0, 1'000)) {
            std::this_thread::sleep_for(1ms);
            q.push({});
        }
    }};

    //
    // Создаём 1 поток, читающий данные из очереди
    //
    std::jthread t4{[&q] {
        for (auto _ : std::views::iota(0, 100)) {
            if (auto res = q.try_pop(); res) {
                std::println("Valued.");
            } else {
                std::println("Queue is empty!");
            }
            std::this_thread::sleep_for(1ms);
        }
    }};
}