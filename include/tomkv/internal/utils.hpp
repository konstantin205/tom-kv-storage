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
#include <climits>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace tomkv {
namespace utils {

#define __TOMKV_ASSERT(condition) assert(condition)

#if defined(__GNUC__) || defined(__clang__)
inline std::uintptr_t clz( unsigned int input ) { return __builtin_clz(input); }
inline std::uintptr_t clz( unsigned long int input ) { return __builtin_clzl(input); }
inline std::uintptr_t clz( unsigned long long int input ) { return __builtin_clzll(input); }
#endif

#if defined(_MSC_VER)
#if _M_X64
#pragma intrinsic(_BitScanReverse64)
#else
#pragma intrinsic(_BitScanReverse)
#endif

inline std::uintptr_t bsr( std::uintptr_t input ) {
    unsigned long result;
#if _M_X64
    _BitScanReverse64(&result, input);
#else
    _BitScanReverse(&result, input);
#endif
    return result;
}
#endif

std::size_t log2(std::size_t input) {
#if defined(__GNUC__) || defined(__clang__)
    // If N is a power of 2 and input<N => (N-1)-x == (N-1)^input
    std::uintptr_t n_bits = sizeof(input) * CHAR_BIT;
    std::size_t result =  std::size_t((n_bits - 1) ^ clz(input));
#elif defined(_MSC_VER)
    std::size_t result = bsr(input);
#else
    // Default implementation
     std::size_t result = 0;
    while (input >>= 1) ++result;
#endif
    return result;
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
