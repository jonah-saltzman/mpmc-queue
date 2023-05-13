#ifndef MPMC_QUEUE
#define MPMC_QUEUE

#include "ring_buffer.hpp"
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T, std::size_t N>
class MPMC_Queue {

public:
    void enqueue(const T& val) {
        {
            std::unique_lock<std::mutex> l(buf_mutex);
            producers.wait(l, [this] { return closed || !buf.is_full(); });
            if (closed) {
                terminate("enqueued to a closed queue");
            }
            buf.push(val);
        }
        consumers.notify_one();
    }

    std::optional<T> dequeue() {
        T ret;
        {
            std::unique_lock<std::mutex> l(buf_mutex);
            consumers.wait(l, [this] { return closed || !buf.is_empty(); });
            if (closed && buf.is_empty()) {
                l.unlock();
                consumers.notify_all();
                return std::nullopt;
            }
            ret = buf.pop();
        }
        producers.notify_one();
        return ret;
    }

    size_t size() {
        std::unique_lock<std::mutex> l(buf_mutex);
        return buf.size();
    }

    void close() {
        {
            std::unique_lock<std::mutex> l(buf_mutex);
            closed = true;
        }
        consumers.notify_all();
    }

private:
    Ring_Buffer<T, N> buf;
    std::mutex buf_mutex;
    std::condition_variable consumers;
    std::condition_variable producers;
    bool closed = false;
};

#endif