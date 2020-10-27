/*
 * MIT License
 *
 * Copyright (c) 2020 Konstantin Boyarinov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __TOMKV_INCLUDE_INTERNAL_UTILS_HPP
#define __TOMKV_INCLUDE_INTERNAL_UTILS_HPP

#include <cmath>
#include <cassert>
#include <chrono>
#include <thread>

namespace tomkv {
namespace utils {

#define __TOMKV_ASSERT(condition) assert(condition)

// TODO: replace with intrinsic-based implementation
std::size_t log2(std::size_t input) {
    return std::log2(input);
}

// Helper, which executes a body if stack-unwinding is in progress
template <typename Body>
class raii_guard {
public:
    raii_guard( const Body& body ) : my_active_tag(true), my_body(body) {}

    ~raii_guard() {
        if (my_active_tag) {
            my_body();
        }
    }

    void release() { my_active_tag = false; }
private:
    bool my_active_tag;
    Body my_body;
}; // class raii_guard

class exponential_backoff {
    static constexpr std::size_t loops_before_pause = 4;
    static constexpr std::size_t loops_before_yield = 16;
    std::size_t counter;

public:
    exponential_backoff() : counter(0) {}

    exponential_backoff(const exponential_backoff&) = delete;
    exponential_backoff& operator=(const exponential_backoff&) = delete;

    void pause() {
        if (counter < loops_before_pause) {
            ++counter;
            return;
        }
        if (counter < loops_before_yield) {
            ++counter;
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            return;
        }
        std::this_thread::yield();
    }

    void reset() {
        counter = 0;
    }
}; // class exponential_backoff

template <typename... Args>
void suppress_unused( Args&&... ) {}

} // namespace utils
} // namespace tomkv

#endif // __TOMKV_INCLUDE_UTILS_HPP
