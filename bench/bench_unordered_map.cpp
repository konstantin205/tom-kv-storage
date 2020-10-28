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

#include "unordered_map_benchmark.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"
#include <iostream>
#include <cassert>

namespace po = boost::program_options;

int main( int argc, char* argv[] ) {
    std::size_t error_percentage = 101;
    std::size_t insert_percentage = error_percentage;
    std::size_t find_percentage = error_percentage;
    std::size_t erase_percentage = error_percentage;
    std::size_t num_threads = 0;
    std::size_t num_elements = 0;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print help message")
        ("insert", po::value<std::size_t>(&insert_percentage), "Percentage of threads that inserts")
        ("find", po::value<std::size_t>(&find_percentage), "Percentage of threads that finds")
        ("erase", po::value<std::size_t>(&erase_percentage), "Percentage of threads that erases")
        ("verbose", "Verbose mode")
        ("num-threads", po::value<std::size_t>(&num_threads)->default_value(std::thread::hardware_concurrency()), "Number of threads")
        ("num-elements", po::value<std::size_t>(&num_elements)->default_value(1000), "Number of elements for insert/lookup/erase")
        ("use-stl", "Use std::unordered_map with std::mutex");
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    if (vm.count("verbose")) {
        verbose = true;
    }

    if (!vm.count("insert")) {
        std::cout << "Error: percentage of insertions is not set" << std::endl;
        return 1;
    }
    if (!vm.count("find")) {
        std::cout << "Error: percentage of finds is not set" << std::endl;
        return 1;
    }
    if (!vm.count("erase")) {
        std::cout << "Error: percentage of erasures is not set" << std::endl;
        return 1;
    }

    if (insert_percentage + find_percentage + erase_percentage != 100) {
        std::cout << "Error: incorrect variables for operations percentage" << std::endl;
        std::cout << "\t" << insert_percentage << " + " << find_percentage << " + " << erase_percentage << " != 100" << std::endl;
        return 1;
    }

    if (vm.count("use-stl")) {
        if (verbose) {
            std::cout << "Testing std::unordered_map" << std::endl;
        }
        basic_stl_umap_benchmark<int, int>(insert_percentage, find_percentage, erase_percentage,
                                           num_threads, num_elements);
    } else {
        if (verbose) {
            std::cout << "Testing tomkv::unordered_map" << std::endl;
        }
        basic_umap_benchmark<int, int>(insert_percentage, find_percentage, erase_percentage,
                                       num_threads, num_elements);
    }
}