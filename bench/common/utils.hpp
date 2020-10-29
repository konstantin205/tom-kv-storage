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

#ifndef __TOMKV_BENCH_UTILS_HPP
#define __TOMKV_BENCH_UTILS_HPP

#include <type_traits>
#include <chrono>
#include <vector>
#include <iostream>
#include <algorithm>

namespace utils {

template <typename RandomAccessIterator>
typename std::iterator_traits<RandomAccessIterator>::value_type median( RandomAccessIterator first, RandomAccessIterator last ) {
    std::sort(first, last);
    auto distance = last - first;
    first += distance / 2 - 1;
    if (distance % 2 == 0) {
        return (*first + *(++first)) / 2;
    } else {
        return *first;
    }
}

template <typename RandomAccessIterator>
typename std::iterator_traits<RandomAccessIterator>::value_type mean( RandomAccessIterator first, RandomAccessIterator last ) {
    auto sum = *first;

    for (auto it = std::next(first); it != last; ++it) {
        sum += *it;
    }

    return sum / (last - first);
}

template <typename Benchmark>
void make_performance_measurements( const Benchmark& benchmark, std::size_t number_of_repetitions = 10 ) {
    std::vector<double> times(number_of_repetitions + 2);

    for (std::size_t i = 0; i < number_of_repetitions + 2; ++i) {
        auto start_timepoint = std::chrono::steady_clock::now();
        benchmark();
        auto finish_timepoint = std::chrono::steady_clock::now();
        times[i] = std::chrono::duration_cast<std::chrono::duration<double>>(finish_timepoint - start_timepoint).count();
    }

    double md = median(std::next(times.begin()), std::prev(times.end()));
    double mn = mean(std::next(times.begin()), std::prev(times.end()));

    std::cout << "Elapsed time (median): " << md << std::endl;
    std::cout << "Elapsed time (mean): " << mn << std::endl;
    std::cout << "Elapsed time (min): " << *(std::next(times.begin())) << std::endl;
    std::cout << "Elapsed time (max): " << *(times.end() - 2) << std::endl;
}

} // namespace utils

#endif // __TOMKV_BENCH_UTILS_HPP
