#include "circular_buffer.hpp"
#include "mpmc_queue.hpp"
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <format>
#include <string>
#include <iostream>

#ifndef DEBUG
#define DEBUG false
#endif

using std::vector;
using std::thread;

void test_ring_buf();
void test_mpmc_queue_1();
void test_mpmc_queue_2();
void test_mpmc_queue_3();
void test_mpmc_queue_4();

void print_multi(const std::string& message);

int main() {
    test_ring_buf();
    test_mpmc_queue_1();
    test_mpmc_queue_2();
    test_mpmc_queue_3();
    test_mpmc_queue_4();
}

void producer(MPMC_Queue<int, 5>& queue, std::atomic_int& produced, int thread, int items) {
    for (int i = 0; i < items; ++i) {
        print_multi(std::format("[{}]: enqueueing {}", thread, i));
        queue.enqueue(i);
        ++produced;
    }
}

void consumer(MPMC_Queue<int, 5>& queue, std::atomic_int& consumed, int thread) {
    int value;
    while (true) {
        print_multi(std::format("[{}]: WAIT", thread));
        auto opt = queue.dequeue();
        if (opt.has_value()) {
            print_multi(std::format("[{}]: dequeued {}", thread, opt.value()));
            ++consumed;
        } else {
            print_multi(std::format("[{}]: DONE", thread));
            return;
        }
    }
}

// more consumers than producers
void test_mpmc_queue_4() {
    constexpr int PRODUCER_COUNT = 10;
    constexpr int CONSUMER_COUNT = 2;
    constexpr int ITEMS_PER_PRODUCER = 1000;
    MPMC_Queue<int, 5> queue;
    std::atomic_int produced(0);
    std::atomic_int consumed(0);

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    int i = 0;

    for (; i < PRODUCER_COUNT; ++i) {
        producers.emplace_back(producer, std::ref(queue), std::ref(produced), i, ITEMS_PER_PRODUCER);
    }

    for (int j = i; j < CONSUMER_COUNT + i; ++j) {
        consumers.emplace_back(consumer, std::ref(queue), std::ref(consumed), j);
    }

    for (auto& p : producers) {
        p.join();
    }

    while (consumed.load() < produced.load()) {
        std::this_thread::yield();
    }

    print_multi(std::format("CLOSING"));
    queue.close();

    for (auto& c : consumers) {
        c.join();
    }

    std::cout << "Produced items: " << produced.load() << std::endl;
    std::cout << "Consumed items: " << consumed.load() << std::endl;
}

// more consumers than producers
void test_mpmc_queue_3() {
    constexpr int PRODUCER_COUNT = 4;
    constexpr int CONSUMER_COUNT = 10;
    constexpr int ITEMS_PER_PRODUCER = 1000;
    MPMC_Queue<int, 5> queue;
    std::atomic_int produced(0);
    std::atomic_int consumed(0);

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    int i = 0;

    for (; i < PRODUCER_COUNT; ++i) {
        producers.emplace_back(producer, std::ref(queue), std::ref(produced), i, ITEMS_PER_PRODUCER);
    }

    for (int j = i; j < CONSUMER_COUNT + i; ++j) {
        consumers.emplace_back(consumer, std::ref(queue), std::ref(consumed), j);
    }

    for (auto& p : producers) {
        p.join();
    }

    while (consumed.load() < produced.load()) {
        std::this_thread::yield();
    }

    print_multi(std::format("CLOSING"));
    queue.close();

    for (auto& c : consumers) {
        c.join();
    }

    std::cout << "Produced items: " << produced.load() << std::endl;
    std::cout << "Consumed items: " << consumed.load() << std::endl;
}


// 3 producers, 3 consumers
void test_mpmc_queue_2() {
    MPMC_Queue<int, 5> queue;
    vector<thread> threads;

    int n_producers = 3;
    int n_consumers = 3;
    int n_produced_per_thread = 10;
    std::atomic_int n_consumed(0);
    std::atomic_int sum_produced(0);
    std::atomic_int sum_consumed(0); 
    int i = 1;
    // producers
    for (; i <= n_producers; ++i) {
        threads.push_back(thread([&sum_produced, &queue, i, n_produced_per_thread] {
            for (int j = 1; j <= n_produced_per_thread; ++j) {
                int n = i * j;
                sum_produced += n;
                queue.enqueue(n);
            }
        }));
    }

    // consumers
    for (int j = i; j <= n_consumers + i; ++j) {
        threads.push_back(thread([&queue, &n_consumed, &sum_consumed, n_produced_per_thread, n_producers, j] {
            while (true) {
                auto v = queue.dequeue();
                if (v.has_value()) {
                    int n = v.value();
                    n_consumed += 1;
                    sum_consumed += n;
                } else {
                    break;
                }
                int consumed = n_consumed.load();
                if (consumed == n_produced_per_thread * n_producers) {
                    queue.close();
                    break;
                }
            }
        }));
    }

    for (thread &t : threads) {
        t.join();
    }
    assert(n_consumed == n_produced_per_thread * n_producers);
    assert(sum_produced == sum_consumed);
}

// one producer, one consumer
void test_mpmc_queue_1() {
    vector<thread> threads;
    vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    vector<int> output(10);
    MPMC_Queue<int, 5> queue;
    
    threads.push_back(thread([&input, &queue] { 
        for (int n : input) {
            queue.enqueue(n);
        }
    }));
    threads.push_back(thread([&output, &queue] { 
        for (int i = 0; i < output.size(); ++i) {
            output[i] = queue.dequeue().value();
        }
    }));
    for (thread &t : threads) {
        t.join();
    }
    assert(input == output);
}

void test_ring_buf() {
    Ring_Buffer<int, 2> buf;
    assert(!buf.is_full());
    assert(buf.is_empty());
    buf.push(1);
    assert(!buf.is_full());
    assert(!buf.is_empty());
    buf.push(2);
    assert(buf.is_full());
    assert(!buf.is_empty());
    int one = buf.pop();
    assert(one == 1);
    assert(!buf.is_full());
    assert(!buf.is_empty());
    int two = buf.pop();
    assert(two == 2);
    assert(!buf.is_full());
    assert(buf.is_empty());
}

void print_multi(const std::string& message) {
    if (DEBUG) {
        static std::mutex print_lock;
        std::unique_lock<std::mutex> l(print_lock);
        printf("%s\n", message.c_str());
        std::cout.flush();
    }
}