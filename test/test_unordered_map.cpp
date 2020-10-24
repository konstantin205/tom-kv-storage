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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "utils.hpp"
#include "tomkv/unordered_map.hpp"
#include <vector>
#include <thread>
#include <algorithm>
#include <random>

TEST_CASE("test serial operations") {
    using key_type = int;
    using mapped_type = int;

    using umap_type = tomkv::unordered_map<key_type, mapped_type>;
    using read_accessor = typename umap_type::read_accessor;
    using write_accessor = typename umap_type::write_accessor;

    // Constructor
    umap_type umap;

    REQUIRE_MESSAGE(umap.empty(), "Newly constructed umap should be empty");
    REQUIRE_MESSAGE(umap.size() == 0, "Newly constructed umap should be empty");

    // Testing insert
    // Test insertion with read accessor
    read_accessor racc;
    bool inserted = umap.emplace(racc, 1, 1);

    REQUIRE_MESSAGE(inserted, "Failed to insert into the empty umap");
    REQUIRE_MESSAGE((racc.key() == 1), "Incorrect value of read accesor after insertion");
    REQUIRE_MESSAGE((racc.mapped() == 1), "Incorrect value of read accessor after insertion");
    REQUIRE_MESSAGE((racc.value().first == 1 && racc.value().second == 1), "Incorrect value of read accessor after insertion");
    racc.release();
    REQUIRE_MESSAGE(umap.size() == 1, "Incorrect table size");
    REQUIRE_MESSAGE(!umap.empty(), "umap is empty after insertion");

    // Test insertion with write accessor
    write_accessor wacc;
    inserted = umap.emplace(wacc, 2, 2);
    REQUIRE_MESSAGE(inserted, "Failed to insert a key which is not presented in the table");
    REQUIRE_MESSAGE((wacc.key() == 2), "Incorrect value of write accessor after insertion");
    REQUIRE_MESSAGE((wacc.mapped() == 2), "Incorrect value of write accessor after insertion");
    REQUIRE_MESSAGE((wacc.value().first == 2 && wacc.value().second == 2), "Incorrect value of write accessor after insertion");
    wacc.mapped() = 4;
    REQUIRE_MESSAGE((wacc.mapped() == 4), "Incorrect value of write accessor after changing");
    auto& value = wacc.value();
    value.second = 8;
    REQUIRE_MESSAGE((wacc.mapped() == 8), "Incorrect value of write accessor after changing");
    wacc.release();
    REQUIRE_MESSAGE((umap.size() == 2), "Incorrect table size");
    REQUIRE_MESSAGE(!umap.empty(), "umap is empty after insertion");

    // Test insertion of duplicated key
    inserted = umap.emplace(racc, 1, 100);
    REQUIRE_MESSAGE(!inserted, "Duplicated key was successfully inserted");
    REQUIRE_MESSAGE((racc.key() == 1), "Incorrect value of read accessor after duplicated key insertion");
    REQUIRE_MESSAGE((racc.mapped() == 1), "Incorrect value of read accessor after duplicated key insertion");
    REQUIRE_MESSAGE((racc.value().first == 1, racc.value().second == 1), "Incorrect value of read accessor after duplicated key insertion");
    racc.release();
    REQUIRE_MESSAGE((umap.size() == 2), "Incorrect table size");
    REQUIRE_MESSAGE(!umap.empty(), "umap is empty after no-changing insertion");

    // Test insertion with no accessor
    inserted = umap.emplace(3, 3);
    REQUIRE_MESSAGE(inserted, "Failed to insert a key which is not in the table");
    REQUIRE_MESSAGE((umap.size() == 3), "Incorrect table size");
    inserted = umap.emplace(3, 300);
    REQUIRE_MESSAGE(!inserted, "Duplicated key was successfully inserted");
    REQUIRE_MESSAGE((umap.size() == 3), "Incorrect table size");

    // Testing find
    // Test finding the key which is presented

    // Test with read accessor
    bool found = umap.find(racc, 2);
    REQUIRE_MESSAGE(found, "Failed to find a key which is presented in the table");
    REQUIRE_MESSAGE(racc.key() == 2, "Incorrect value of read accessor after finding");
    REQUIRE_MESSAGE(racc.mapped() == 8, "Incorrect value of read accessor after finding");
    REQUIRE_MESSAGE((racc.value().first == 2 && racc.value().second == 8), "Incorrect value of read accessor after finding");
    REQUIRE_MESSAGE(umap.size() == 3, "Incorrect table size");
    racc.release();

    // Test with write accessor
    found = umap.find(wacc, 2);
    REQUIRE_MESSAGE(found, "Failed to find a key which is presented in the table");
    REQUIRE_MESSAGE(wacc.key() == 2, "Incorrect value of write accessor after finding");
    REQUIRE_MESSAGE(wacc.mapped() == 8, "Incorrect value of write accessor after finding");
    REQUIRE_MESSAGE((wacc.value().first == 2 && wacc.value().second == 8), "Incorrect value of write accessor after finding");
    wacc.mapped() = 4;
    REQUIRE_MESSAGE(wacc.mapped() == 4, "Incorrect value of write accessor after changing");
    wacc.release();
    REQUIRE_MESSAGE(umap.size() == 3, "Incorrect table size");

    // Re-find after changing mapped
    found = umap.find(racc, 2);
    REQUIRE_MESSAGE(racc.mapped() == 4, "Incorrect value of read accessor after changing and re-finding");
    racc.release();

    // Test finding the key which is not presented
    found = umap.find(racc, 100);
    REQUIRE_MESSAGE(!found, "Key which is not presented was successfully found");
    REQUIRE_MESSAGE(umap.size() == 3, "Incorrect table size");

    // Test erasure

    // Test erasure by key which is presented in the table
    bool erased = umap.erase(1);
    REQUIRE_MESSAGE(erased, "Failed to erase the key which is presented in the table");

    // Try to find the key
    found = umap.find(racc, 1);
    REQUIRE_MESSAGE(!found, "Erased element was successfully found");

    // Test erasure by key which is not presented in the table
    erased = umap.erase(100);
    REQUIRE_MESSAGE(!erased, "Element with the key which is not presented in the table was removed");

    // Test erasure by write accessor
    found = umap.find(wacc, 2);
    REQUIRE(found);

    umap.erase(wacc);

    // Try to re- find
    found = umap.find(racc, 2);
    REQUIRE_MESSAGE(!found, "Removed element was successfully found");

    // Test rehashing validity - insert a lot of items
    tomkv::unordered_map<int, int> hl_umap;

    // At least one rehashing should accure
    for (int i = 0; i < 10000; ++i) {
        hl_umap.emplace(i, i);
    }

    REQUIRE_MESSAGE(hl_umap.size() == 10000, "Incorrect high-load unordered_map size");

    // Check map validity
    for (int i = 0; i < 10000; ++i) {
        read_accessor racc;
        bool f = hl_umap.find(racc, i);
        REQUIRE_MESSAGE(f, "Unable to find element in high-load unordered_map");
        REQUIRE_MESSAGE(racc.key() == i, "Incorrect element returned from find operation on high-load unordered_map");
        REQUIRE_MESSAGE(racc.mapped() == i, "Incorrect element returned fromt find operation on high-load unordered_map");
    }
}

TEST_CASE("test parallel operations") {
    using key_type = int;
    using mapped_type = int;

    using umap_type = tomkv::unordered_map<key_type, mapped_type>;
    using read_accessor = typename umap_type::read_accessor;
    using write_accessor = typename umap_type::write_accessor;

    // Constructor
    {
    umap_type umap;

    std::vector<std::thread> thread_pool;
    std::vector<int> values(10000);

    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = i;
    }

    // Test parallel insertion with no accessor
    // Each thread inserts 10000 items into hash table
    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        thread_pool.emplace_back([i, &values, &umap] {
            for (auto& item : values) {
                umap.emplace(item, item);
            }
        });
    }

    for (auto& thr : thread_pool) {
        thr.join();
    }

    REQUIRE_MESSAGE(umap.size() == values.size(), "Incorrect umap size after parallel insertions");
    for (std::size_t i = 0; i < values.size(); ++i) {
        read_accessor acc;
        bool found = umap.find(acc, values[i]);
        REQUIRE_MESSAGE(found, "Element cannot be found in the umap after parallel insertions");
        REQUIRE_MESSAGE(acc.key() == values[i], "Incorrect key in found element after parallel insertions");
        REQUIRE_MESSAGE(acc.mapped() == values[i], "Incorrect mapped in found element after parallel insertions");
    }

    } // end of the thread scope

    {
        using atomic_umap_type = tomkv::unordered_map<key_type, std::atomic<mapped_type>>;

        using atomic_umap_read_accessor = typename atomic_umap_type::read_accessor;
        using atomic_umap_write_accessor = typename atomic_umap_type::write_accessor;

        atomic_umap_type umap;
        // Test 50% insertions and 50% of finds (for pre-existing elements)
        std::vector<std::thread> thread_pool;

        // Fill the table
        for (std::size_t i = 0; i < 1000; ++i) {
            umap.emplace(i, i);
        }

        std::atomic<std::size_t> finding_threads = 0;

        for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            thread_pool.emplace_back([i, &umap, &finding_threads] {
                if (i % 2 == 0) {
                    // Insertion thread
                    for (std::size_t i = 0; i < 5000; ++i) {
                        umap.emplace(i + 1000, i);
                    }
                } else {
                    // Finding thread
                    ++finding_threads;
                    for (std::size_t i = 0; i < 1000; ++i) {
                        atomic_umap_write_accessor wacc;
                        bool found = umap.find(wacc, i);
                        REQUIRE_MESSAGE(found, "Cannot found pre-existing element");
                        REQUIRE_MESSAGE(wacc.key() == i, "Incorrect element found");
                        ++wacc.mapped();
                    }
                }
            });
        }

        for (auto& thr : thread_pool) {
            thr.join();
        }

        // 5000 of new elements and 1000 of pre-existing
        REQUIRE_MESSAGE(umap.size() == 6000, "Incorrect umap size");

        // Check pre-existing
        for (std::size_t i = 0; i < 1000; ++i) {
            atomic_umap_read_accessor racc;
            bool found = umap.find(racc, i);
            REQUIRE_MESSAGE(found, "Cannot find pre-existing element");
            REQUIRE_MESSAGE(racc.key() == i, "Incorrect key in pre-existing element");
            REQUIRE_MESSAGE(racc.mapped() == i + finding_threads, "Incorrect mapped in pre-existing element (changed)");
        }

        // Check new elements
        for (std::size_t i = 0; i < 1000; ++i) {
            atomic_umap_read_accessor racc;
            bool found = umap.find(racc, i + 1000);
            REQUIRE_MESSAGE(found, "Cannot find new element");
            REQUIRE_MESSAGE(racc.key() == i + 1000, "Incorrect key in new element");
            REQUIRE_MESSAGE(racc.mapped() == i, "Incorrect mapped in pre-existing element");
        }
    } // end of the thread scope

    {
        // Test 50% insertions with 50% erasures (of pre-existing elements)
        umap_type umap;

        std::vector<std::thread> thread_pool;
        std::vector<int> pre_existing_elements(1000);

        for (std::size_t i = 0; i < pre_existing_elements.size(); ++i) {
            pre_existing_elements[i] = i;
            umap.emplace(i, i);
        }

        for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            thread_pool.emplace_back([i, &umap, pre_existing_elements]() mutable {
                std::mt19937 rng;
                std::shuffle(pre_existing_elements.begin(), pre_existing_elements.end(), rng);
                if (i % 2 == 0) {
                    for (auto& item : pre_existing_elements) {
                        umap.emplace(item + 1000, item);
                    }
                } else {
                    for (auto& item : pre_existing_elements) {
                        umap.erase(item);
                    }
                }
            });
        }

        for (auto& thr : thread_pool) {
            thr.join();
        }

        REQUIRE_MESSAGE(umap.size() == pre_existing_elements.size(), "Incorrect umap size");

        // Check pre-existing elements
        for (auto& item : pre_existing_elements) {
            read_accessor racc;
            REQUIRE_MESSAGE(!umap.find(racc, item), "Erased item was successfully found");
        }

        // Check new elements
        for (auto& item : pre_existing_elements) {
            read_accessor racc;
            bool found = umap.find(racc, item + 1000);
            REQUIRE_MESSAGE(racc.key() == item + 1000, "Incorrect key for found element (new)");
            REQUIRE_MESSAGE(racc.mapped() == item, "Incorrect mapped for found element (new)");
        }
    } // end of the thread scope
}

TEST_CASE("test memory leaks") {
    using map_type = tomkv::unordered_map<int, int, std::hash<int>, std::equal_to<int>,
                                          utils::counting_allocator<std::pair<const int, int>>>;

    utils::counting_allocator<std::pair<const int, int>> count_alloc;

    {
    map_type umap(count_alloc);

    // Emplace some new elements with no accessor
    for (int i = 0; i < 5000; ++i) {
        umap.emplace(i, i);
        umap.emplace(i, i); // Emplace duplicated
    }

    // Emplace some new elements with read accessor
    for (int i = 5000; i < 10000; ++i) {
        typename map_type::read_accessor racc;
        umap.emplace(racc, i, i);
        umap.emplace(racc, i, i); // Emplace duplicated
    }

    // Emplace some new elements with write accessor
    for (int i = 10000; i < 15000; ++i) {
        typename map_type::write_accessor wacc;
        umap.emplace(wacc, i, i);
        umap.emplace(wacc, i, i); // Emplace duplicated
    }

    // Erase some existing elements
    for (int i = 0; i < 1000; ++i) {
        umap.erase(i);
        umap.erase(i); // Erase duplicated
    }

    } // umap is destroyed here

    REQUIRE_MESSAGE(count_alloc.elements_allocated != 0, "Incorrect test setup");
    REQUIRE_MESSAGE(count_alloc.allocations == count_alloc.deallocations, "Memory leak: number of allocate and deallocate calls should be equal");
    REQUIRE_MESSAGE(count_alloc.elements_allocated == count_alloc.elements_deallocated,
                    "Memory leak: number of elements allocated and the number of elements deallocated should be equal");
    REQUIRE_MESSAGE(count_alloc.elements_constructed == count_alloc.elements_destroyed,
                    "Memory leak: number of elements constructed and the number of elements destroyed should be equal");
    count_alloc.reset();

    // TODO: Add test with exception in the constructor
}
