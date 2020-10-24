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

#ifndef __TOMKV_TEST_UTILS_HPP
#define __TOMKV_TEST_UTILS_HPP

#include <memory>

namespace utils {
namespace internal {

struct counting_allocator_base {
    static std::size_t allocations;
    static std::size_t deallocations;
    static std::size_t elements_allocated;
    static std::size_t elements_deallocated;
    static std::size_t elements_constructed;
    static std::size_t elements_destroyed;
};

std::size_t counting_allocator_base::allocations = 0;
std::size_t counting_allocator_base::deallocations = 0;
std::size_t counting_allocator_base::elements_allocated = 0;
std::size_t counting_allocator_base::elements_deallocated = 0;
std::size_t counting_allocator_base::elements_constructed = 0;
std::size_t counting_allocator_base::elements_destroyed = 0;

} // namespace internal

template <typename T>
struct counting_allocator : public internal::counting_allocator_base {
    using value_type = T;

    counting_allocator() = default;

    template <typename U>
    counting_allocator( const counting_allocator<U>& ) {}

    static void reset() {
        allocations = 0;
        deallocations = 0;
        elements_allocated = 0;
        elements_deallocated = 0;
        elements_constructed = 0;
        elements_destroyed = 0;
    }

    T* allocate( std::size_t n ) {
        ++allocations;
        elements_allocated += n;
        return reinterpret_cast<T*>(std::malloc(n * sizeof(T)));
    }

    void deallocate( T* ptr, std::size_t n ) {
        ++deallocations;
        elements_deallocated += n;
        std::free(reinterpret_cast<void*>(ptr));
    }

    template <typename... Args>
    void construct( T* ptr, Args&&... args ) {
        ++elements_constructed;
        ::new(ptr) T(std::forward<Args>(args)...);
    }

    void destroy( T* ptr ) {
        ++elements_destroyed;
        ptr->~T();
    }
}; // class counting_allocator

} // namespace utils

#endif // __TOMKV_TEST_UTILS_HPP
