#ifndef RING_BUFFER
#define RING_BUFFER

#include <cstdlib>
#include <array>
#include <cstdio>
#include <exception>

void terminate(const char msg[]) {
    printf("err: %s\n", msg);
    std::terminate();
}

template<typename T, std::size_t N>
class Ring_Buffer {
public:
    void push(const T& val) {
        if (is_full())
            terminate("tried to push when buffer full");
        int new_back = (back + 1) % N;
        buf[back] = val;
        back = new_back;
        curr_elements += 1;
    }
    T pop() {
        if (is_empty())
            terminate("tried to pop when buffer empty");
        int new_front = (front + 1) % N;
        T ret = buf[front];
        front = new_front;
        curr_elements -= 1;
        return ret;
    }

    bool is_full() {
        return curr_elements == N;
    }

    bool is_empty() {
        return curr_elements == 0;
    }

    size_t size() {
        return curr_elements;
    }
    
private:
    size_t front = 0;
    size_t back = 0;
    size_t curr_elements = 0;
    std::array<T, N> buf;
};

#endif