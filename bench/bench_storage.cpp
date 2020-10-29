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

#include "common/utils.hpp"
#include <tomkv/storage.hpp>
#include <tomkv/tom_management.hpp>
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <string>
#include <atomic>

namespace po = boost::program_options;
namespace pt = boost::property_tree;

template <typename T>
void suppress_unused( T&& ) {}

bool verbose = true;

template <typename Key, typename Mapped>
void run_benchmark( std::size_t mount_percentage,
                    std::size_t read_percentage,
                    std::size_t write_percentage,
                    std::size_t insert_percentage,
                    std::size_t num_threads,
                    std::size_t num_operations )
{
    assert(mount_percentage + read_percentage + write_percentage + insert_percentage);
    std::size_t mount_threads = std::size_t(num_threads / 100. * mount_percentage);
    std::size_t read_threads = std::size_t(num_threads / 100. * read_percentage);
    std::size_t write_threads = std::size_t(num_threads / 100. * write_percentage);
    std::size_t insert_threads = std::size_t(num_threads / 100. * insert_percentage);

    if (verbose) {
        std::cout << "Info:" << std::endl;
        std::cout << "\tTotal number of threads = " << num_threads << std::endl;
        std::cout << "\tNumber of threads for mounting = " << mount_threads << std::endl;
        std::cout << "\tNumber of threads for reading = " << read_threads << std::endl;
        std::cout << "\tNumber of threads for writing = " << write_threads << std::endl;
        std::cout << "\tNumber of threads for inserting = " << insert_threads << std::endl;
        std::cout << "\tNumber of operations per thread = " << num_operations << std::endl;
    }

    auto benchmark_body = [&] {
        std::string tom_name = "tom.xml";
        tomkv::create_empty_tom("tom.xml");

        pt::ptree tree;
        pt::read_xml(tom_name, tree);

        tree.add("tom.root.a.key", Key(1));
        tree.add("tom.root.a.mapped", Mapped(100));

        tree.add("tom.root.a.b.key", Key(2));
        tree.add("tom.root.a.b.mapped", Mapped(200));

        pt::write_xml(tom_name, tree);

        tomkv::storage<Key, Mapped> st;
        std::vector<std::thread> thread_pool;

        std::string mount_path = "mnt";
        std::string real_path = "a";

        std::atomic<bool> start_allowed = false;
        st.mount(mount_path, tom_name, real_path);

        // Fill the mount threads
        for (std::size_t i = 0; i < mount_threads; ++i) {
            thread_pool.emplace_back([num_operations, &st, &mount_path, &real_path, &tom_name, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < num_operations; ++i) {
                    st.mount(mount_path, tom_name, real_path + '/' + std::to_string(i));
                }
            });
        }

        // Fill the read threads
        for (std::size_t i = 0; i < read_threads; ++i) {
            thread_pool.emplace_back([num_operations, &st, &mount_path, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < num_operations; ++i) {
                    volatile auto values = st.value(mount_path + '/' + std::to_string(i));
                    suppress_unused(values);
                }
            });
        }

        // Fill the write threads
        for (std::size_t i = 0; i < write_threads; ++i) {
            thread_pool.emplace_back([num_operations, &st, &mount_path, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < num_operations; ++i) {
                    volatile auto count = st.set_value(mount_path + '/' + std::to_string(i), std::pair{Key(42), Mapped(4242)});
                    suppress_unused(count);
                }
            });
        }

        // Fill the insert threads
        for (std::size_t i = 0; i < insert_threads; ++i) {
            thread_pool.emplace_back([num_operations, &st, &mount_path, &start_allowed] {
                while (start_allowed.load(std::memory_order_acquire) == false) {
                    // Spin until the start is allowed
                }

                for (std::size_t i = 0; i < num_operations; ++i) {
                    st.insert(mount_path + "/" + std::to_string(i), std::pair{Key(33), Mapped(3333)});
                }
            });
        }

        start_allowed.store(true, std::memory_order_release);

        for (auto& thr : thread_pool) {
            thr.join();
        }

        tomkv::remove_tom(tom_name);
    }; // End of the benchmark body

    utils::make_performance_measurements(benchmark_body);
}

int main( int argc, char* argv[] ) {
    std::size_t error_percentage = 101;
    std::size_t mount_percentage = error_percentage;
    std::size_t read_percentage = error_percentage;
    std::size_t write_percentage = error_percentage;
    std::size_t insert_percentage = error_percentage;
    std::size_t num_threads = 0;
    std::size_t num_operations = 0;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print help message")
        ("mount", po::value<std::size_t>(&mount_percentage), "Percentage of threads that mounts new paths")
        ("read", po::value<std::size_t>(&read_percentage), "Percentage of threads that reads elements")
        ("write", po::value<std::size_t>(&write_percentage), "Percentage of threads that modifyes elements")
        ("insert", po::value<std::size_t>(&insert_percentage), "Percentage of threads that inserts new nodes")
        ("verbose", "Verbose mode")
        ("num-threads", po::value<std::size_t>(&num_threads)->default_value(std::thread::hardware_concurrency()), "Number of threads")
        ("num-operations", po::value<std::size_t>(&num_operations)->default_value(10), "Number of mount/read/write/insert operations per thread")
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

    if (!vm.count("mount")) {
        std::cout << "Error: percentage of mounts is not set" << std::endl;
        return 1;
    }

    if (!vm.count("read")) {
        std::cout << "Error: percentage of reads is not set" << std::endl;
        return 1;
    }

    if (!vm.count("write")) {
        std::cout << "Error: percentage of writes is not set" << std::endl;
        return 1;
    }

    if (!vm.count("insert")) {
        std::cout << "Error: percentage of inserts is not set" << std::endl;
        return 1;
    }

    if (mount_percentage + read_percentage + write_percentage + insert_percentage != 100) {
        std::cout << "Error: incorrect variables for operations percentage" << std::endl;
        std::cout << "\t" << mount_percentage << " + " << read_percentage << " + " << write_percentage << " + " << insert_percentage << " != 100" << std::endl;
        return 1;
    }

    run_benchmark<int, int>(mount_percentage, read_percentage,
                            write_percentage, insert_percentage,
                            num_threads, num_operations);
}