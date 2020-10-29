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

#ifndef __TOMKV_BENCH_UNORDERED_MAP_BENCHMARK_HPP
#define __TOMKV_BENCH_UNORDERED_MAP_BENCHMARK_HPP

#include "utils.hpp"
#include <tomkv/unordered_map.hpp>
#include <thread>
#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <tuple>
#include <unordered_map>
#include <mutex>

static bool verbose = false;

template <typename T>
void suppress_unused( T&& ) {}

template <typename Key, typename Mapped,
          typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Mapped>>>
void basic_stl_umap_benchmark( std::size_t insert_percentage,
                               std::size_t find_percentage,
                               std::size_t erase_percentage,
                               std::size_t num_threads = std::thread::hardware_concurrency(),
                               std::size_t number_of_elements_per_thread = 1000 )
{
    assert(insert_percentage + find_percentage + erase_percentage == 100);
    std::size_t insert_threads = std::size_t(num_threads / 100. * insert_percentage);
    std::size_t find_threads = std::size_t(num_threads / 100. * find_percentage);
    std::size_t erase_threads = std::size_t(num_threads / 100. * erase_percentage);

    if (verbose) {
        std::cout << "Info: " << std::endl;
        std::cout << "\tTotal number of threads = " << num_threads << std::endl;
        std::cout << "\tNumber of threads for insertion = " << insert_threads << std::endl;
        std::cout << "\tNumber of threads for lookup = " << find_threads << std::endl;
        std::cout << "\tNumber of threads for erasure = " << erase_threads << std::endl;
        std::cout << "\tNumber of elements = " << number_of_elements_per_thread << std::endl;
    }

    auto benchmark_body = [&] {
        using umap_type = std::unordered_map<Key, Mapped, Hasher, KeyEqual, Allocator>;
        using mutex_type = std::mutex;
        using lock_type = std::unique_lock<mutex_type>;

        umap_type umap;
        mutex_type mutex;

        std::atomic<bool> start_allowed = false;

        std::vector<std::thread> thread_pool;

        // Fill the insertion threads
        for (std::size_t i = 0; i < insert_threads; ++i) {
            thread_pool.emplace_back([number_of_elements_per_thread, &umap, &mutex, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < number_of_elements_per_thread; ++i) {
                    lock_type lc(mutex);
                    umap.emplace(std::piecewise_construct,
                                std::forward_as_tuple(i),
                                std::tuple<>());
                }
            });
        }

        // Fill the lookup threads
        for (std::size_t i = 0; i < find_threads; ++i) {
            thread_pool.emplace_back([number_of_elements_per_thread, &umap, &mutex, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < number_of_elements_per_thread; ++i) {
                    lock_type lc(mutex);
                    volatile typename decltype(umap)::iterator it = umap.find(Key(i));
                    suppress_unused(it);
                }
            });
        }

        // Fill the erasure threads
        for (std::size_t i = 0; i < erase_threads; ++i) {
            thread_pool.emplace_back([number_of_elements_per_thread, &umap, &mutex, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < number_of_elements_per_thread; ++i) {
                    lock_type lc(mutex);
                    volatile std::size_t count = umap.erase(Key(i));
                }
            });
        }

        start_allowed.store(true, std::memory_order_release);

        for (auto& thr : thread_pool) {
            thr.join();
        }
    }; // End of the benchmark body
    utils::make_performance_measurements(benchmark_body);
}

template <typename Key, typename Mapped,
          typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Mapped>>>
void basic_umap_benchmark( std::size_t insert_percentage,
                           std::size_t find_percentage,
                           std::size_t erase_percentage,
                           std::size_t num_threads = std::thread::hardware_concurrency(),
                           std::size_t number_of_elements_per_thread = 1000 )
{
    assert(insert_percentage + find_percentage + erase_percentage == 100);
    std::size_t insert_threads = std::size_t(num_threads / 100. * insert_percentage);
    std::size_t find_threads = std::size_t(num_threads / 100. * find_percentage);
    std::size_t erase_threads = std::size_t(num_threads / 100. * erase_percentage);

    if (verbose) {
        std::cout << "Info: " << std::endl;
        std::cout << "\tTotal number of threads = " << num_threads << std::endl;
        std::cout << "\tNumber of threads for insertion = " << insert_threads << std::endl;
        std::cout << "\tNumber of threads for lookup = " << find_threads << std::endl;
        std::cout << "\tNumber of threads for erasure = " << erase_threads << std::endl;
        std::cout << "\tNumber of elements = " << number_of_elements_per_thread << std::endl;
    }

    auto benchmark_body = [&] {
        using umap_type = tomkv::unordered_map<Key, Mapped, Hasher, KeyEqual, Allocator>;

        umap_type umap;

        std::atomic<bool> start_allowed = false;

        std::vector<std::thread> thread_pool;

        // Fill the insertion threads
        for (std::size_t i = 0; i < insert_threads; ++i) {
            thread_pool.emplace_back([number_of_elements_per_thread, &umap, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < number_of_elements_per_thread; ++i) {
                    typename decltype(umap)::read_accessor racc;
                    umap.emplace(racc, std::piecewise_construct,
                                 std::forward_as_tuple(i),
                                 std::tuple<>());
                }
            });
        }

        // Fill the lookup threads
        for (std::size_t i = 0; i < find_threads; ++i) {
            thread_pool.emplace_back([number_of_elements_per_thread, &umap, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < number_of_elements_per_thread; ++i) {
                    typename decltype(umap)::read_accessor racc;
                    umap.find(racc, Key(i));
                }
            });
        }

        for (std::size_t i = 0; i < erase_threads; ++i) {
            thread_pool.emplace_back([number_of_elements_per_thread, &umap, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < number_of_elements_per_thread; ++i) {
                    umap.erase(Key(i));
                }
            });
        }

        start_allowed.store(true, std::memory_order_release);

        for (auto& thr : thread_pool) {
            thr.join();
        }
    }; // End of the benchmark body

    utils::make_performance_measurements(benchmark_body);
}

#endif // __TOMKV_BENCH_UNORDERED_MAP_BENCHMARK_HPP
