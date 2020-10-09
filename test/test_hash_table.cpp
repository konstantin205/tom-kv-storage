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

#include "tomkv/hash_table.hpp"
#include <vector>
#include <thread>
#include <algorithm>
#include <random>

TEST_CASE("test serial operations") {
    using key_type = int;
    using mapped_type = int;
    using hasher = std::hash<int>;
    using key_eq = std::equal_to<int>;
    using allocator_type = std::allocator<int>;

    hasher h;
    key_eq eq;
    allocator_type alloc;

    using hashtable_type = tomkv::hash_table<key_type, mapped_type,
                                             hasher, key_eq, allocator_type>;
    using read_accessor = typename hashtable_type::read_accessor;
    using write_accessor = typename hashtable_type::write_accessor;

    // Constructor
    hashtable_type hashtable(h, eq, alloc);

    REQUIRE_MESSAGE(hashtable.empty(), "Newly constructed hashtable should be empty");
    REQUIRE_MESSAGE(hashtable.size() == 0, "Newly constructed hashtable should be empty");

    // Testing insert
    // Test insertion with read accessor
    read_accessor racc;
    bool inserted = hashtable.emplace(racc, 1, 1);

    REQUIRE_MESSAGE(inserted, "Failed to insert into the empty hashtable");
    REQUIRE_MESSAGE((racc.key() == 1), "Incorrect value of read accesor after insertion");
    REQUIRE_MESSAGE((racc.mapped() == 1), "Incorrect value of read accessor after insertion");
    REQUIRE_MESSAGE((racc.value().first == 1 && racc.value().second == 1), "Incorrect value of read accessor after insertion");
    racc.release();
    REQUIRE_MESSAGE(hashtable.size() == 1, "Incorrect table size");
    REQUIRE_MESSAGE(!hashtable.empty(), "hashtable is empty after insertion");

    // Test insertion with write accessor
    write_accessor wacc;
    inserted = hashtable.emplace(wacc, 2, 2);
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
    REQUIRE_MESSAGE((hashtable.size() == 2), "Incorrect table size");
    REQUIRE_MESSAGE(!hashtable.empty(), "hashtable is empty after insertion");

    // Test insertion of duplicated key
    inserted = hashtable.emplace(racc, 1, 100);
    REQUIRE_MESSAGE(!inserted, "Duplicated key was successfully inserted");
    REQUIRE_MESSAGE((racc.key() == 1), "Incorrect value of read accessor after duplicated key insertion");
    REQUIRE_MESSAGE((racc.mapped() == 1), "Incorrect value of read accessor after duplicated key insertion");
    REQUIRE_MESSAGE((racc.value().first == 1, racc.value().second == 1), "Incorrect value of read accessor after duplicated key insertion");
    racc.release();
    REQUIRE_MESSAGE((hashtable.size() == 2), "Incorrect table size");
    REQUIRE_MESSAGE(!hashtable.empty(), "hashtable is empty after no-changing insertion");

    // Test insertion with no accessor
    inserted = hashtable.emplace(3, 3);
    REQUIRE_MESSAGE(inserted, "Failed to insert a key which is not in the table");
    REQUIRE_MESSAGE((hashtable.size() == 3), "Incorrect table size");
    inserted = hashtable.emplace(3, 300);
    REQUIRE_MESSAGE(!inserted, "Duplicated key was successfully inserted");
    REQUIRE_MESSAGE((hashtable.size() == 3), "Incorrect table size");

    // Testing find
    // Test finding the key which is presented

    // Test with read accessor
    bool found = hashtable.find(racc, 2);
    REQUIRE_MESSAGE(found, "Failed to find a key which is presented in the table");
    REQUIRE_MESSAGE(racc.key() == 2, "Incorrect value of read accessor after finding");
    REQUIRE_MESSAGE(racc.mapped() == 8, "Incorrect value of read accessor after finding");
    REQUIRE_MESSAGE((racc.value().first == 2 && racc.value().second == 8), "Incorrect value of read accessor after finding");
    REQUIRE_MESSAGE(hashtable.size() == 3, "Incorrect table size");
    racc.release();

    // Test with write accessor
    found = hashtable.find(wacc, 2);
    REQUIRE_MESSAGE(found, "Failed to find a key which is presented in the table");
    REQUIRE_MESSAGE(wacc.key() == 2, "Incorrect value of write accessor after finding");
    REQUIRE_MESSAGE(wacc.mapped() == 8, "Incorrect value of write accessor after finding");
    REQUIRE_MESSAGE((wacc.value().first == 2 && wacc.value().second == 8), "Incorrect value of write accessor after finding");
    wacc.mapped() = 4;
    REQUIRE_MESSAGE(wacc.mapped() == 4, "Incorrect value of write accessor after changing");
    wacc.release();
    REQUIRE_MESSAGE(hashtable.size() == 3, "Incorrect table size");

    // Re-find after changing mapped
    found = hashtable.find(racc, 2);
    REQUIRE_MESSAGE(racc.mapped() == 4, "Incorrect value of read accessor after changing and re-finding");
    racc.release();

    // Test finding the key which is not presented
    found = hashtable.find(racc, 100);
    REQUIRE_MESSAGE(!found, "Key which is not presented was successfully found");
    REQUIRE_MESSAGE(hashtable.size() == 3, "Incorrect table size");

    // Test erasure

    // Test erasure by key which is presented in the table
    bool erased = hashtable.erase(1);
    REQUIRE_MESSAGE(erased, "Failed to erase the key which is presented in the table");

    // Try to find the key
    found = hashtable.find(racc, 1);
    REQUIRE_MESSAGE(!found, "Erased element was successfully found");

    // Test erasure by key which is not presented in the table
    erased = hashtable.erase(100);
    REQUIRE_MESSAGE(!erased, "Element with the key which is not presented in the table was removed");

    // Test erasure by write accessor
    found = hashtable.find(wacc, 2);
    REQUIRE(found);

    hashtable.erase(wacc);

    // Try to re- find
    found = hashtable.find(racc, 2);
    REQUIRE_MESSAGE(!found, "Removed element was successfully found");
}

TEST_CASE("test parallel operations") {
    using key_type = int;
    using mapped_type = int;
    using hasher = std::hash<int>;
    using key_eq = std::equal_to<int>;
    using allocator_type = std::allocator<int>;

    hasher h;
    key_eq eq;
    allocator_type alloc;

    using hashtable_type = tomkv::hash_table<key_type, mapped_type,
                                             hasher, key_eq, allocator_type>;
    using read_accessor = typename hashtable_type::read_accessor;
    using write_accessor = typename hashtable_type::write_accessor;

    // Constructor
    {
    hashtable_type hashtable(h, eq, alloc);

    std::vector<std::thread> thread_pool;
    std::vector<int> values(10000);

    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = i;
        // hashtable.emplace(i % 4, i);
    }

    // Test parallel insertion with no accessor
    // Each thread inserts 10000 items into hash table
    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        thread_pool.emplace_back([i, &values, &hashtable] {
            for (auto& item : values) {
                hashtable.emplace(item, item);
            }
        });
    }

    for (auto& thr : thread_pool) {
        thr.join();
    }

    REQUIRE_MESSAGE(hashtable.size() == values.size(), "Incorrect hashtable size after parallel insertions");
    for (std::size_t i = 0; i < values.size(); ++i) {
        read_accessor acc;
        bool found = hashtable.find(acc, values[i]);
        REQUIRE_MESSAGE(found, "Element cannot be found in the hashtable after parallel insertions");
        REQUIRE_MESSAGE(acc.key() == values[i], "Incorrect key in found element after parallel insertions");
        REQUIRE_MESSAGE(acc.mapped() == values[i], "Incorrect mapped in found element after parallel insertions");
    }

    } // end of the thread scope

    {
        using atomic_hashtable_type = tomkv::hash_table<key_type, std::atomic<mapped_type>,
                                                        hasher, key_eq, allocator_type>;

        using atomic_hashtable_read_accessor = typename atomic_hashtable_type::read_accessor;
        using atomic_hashtable_write_accessor = typename atomic_hashtable_type::write_accessor;

        atomic_hashtable_type hashtable(h, eq, alloc);
        // Test 50% insertions and 50% of finds (for pre-existing elements)
        std::vector<std::thread> thread_pool;

        // Fill the table
        for (std::size_t i = 0; i < 1000; ++i) {
            hashtable.emplace(i, i);
        }

        std::atomic<std::size_t> finding_threads = 0;

        for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            thread_pool.emplace_back([i, &hashtable, &finding_threads] {
                if (i % 2 == 0) {
                    // Insertion thread
                    for (std::size_t i = 0; i < 5000; ++i) {
                        hashtable.emplace(i + 1000, i);
                    }
                } else {
                    // Finding thread
                    ++finding_threads;
                    for (std::size_t i = 0; i < 1000; ++i) {
                        atomic_hashtable_write_accessor wacc;
                        bool found = hashtable.find(wacc, i);
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
        REQUIRE_MESSAGE(hashtable.size() == 6000, "Incorrect hashtable size");

        // Check pre-existing
        for (std::size_t i = 0; i < 1000; ++i) {
            atomic_hashtable_read_accessor racc;
            bool found = hashtable.find(racc, i);
            REQUIRE_MESSAGE(found, "Cannot find pre-existing element");
            REQUIRE_MESSAGE(racc.key() == i, "Incorrect key in pre-existing element");
            REQUIRE_MESSAGE(racc.mapped() == i + finding_threads, "Incorrect mapped in pre-existing element (changed)");
        }

        // Check new elements
        for (std::size_t i = 0; i < 1000; ++i) {
            atomic_hashtable_read_accessor racc;
            bool found = hashtable.find(racc, i + 1000);
            REQUIRE_MESSAGE(found, "Cannot find new element");
            REQUIRE_MESSAGE(racc.key() == i + 1000, "Incorrect key in new element");
            REQUIRE_MESSAGE(racc.mapped() == i, "Incorrect mapped in pre-existing element");
        }
    } // end of the thread scope

    {
        // Test 50% insertions with 50% erasures (of pre-existing elements)
        hashtable_type hashtable(h, eq, alloc);

        std::vector<std::thread> thread_pool;
        std::vector<int> pre_existing_elements(1000);

        for (std::size_t i = 0; i < pre_existing_elements.size(); ++i) {
            pre_existing_elements[i] = i;
            hashtable.emplace(i, i);
        }

        for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            thread_pool.emplace_back([i, &hashtable, pre_existing_elements]() mutable {
                std::mt19937 rng;
                std::shuffle(pre_existing_elements.begin(), pre_existing_elements.end(), rng);
                if (i % 2 == 0) {
                    for (auto& item : pre_existing_elements) {
                        hashtable.emplace(item + 1000, item);
                    }
                } else {
                    for (auto& item : pre_existing_elements) {
                        hashtable.erase(item);
                    }
                }
            });
        }

        for (auto& thr : thread_pool) {
            thr.join();
        }

        REQUIRE_MESSAGE(hashtable.size() == pre_existing_elements.size(), "Incorrect hashtable size");

        // Check pre-existing elements
        for (auto& item : pre_existing_elements) {
            read_accessor racc;
            REQUIRE_MESSAGE(!hashtable.find(racc, item), "Erased item was successfully found");
        }

        // Check new elements
        for (auto& item : pre_existing_elements) {
            read_accessor racc;
            bool found = hashtable.find(racc, item + 1000);
            REQUIRE_MESSAGE(racc.key() == item + 1000, "Incorrect key for found element (new)");
            REQUIRE_MESSAGE(racc.mapped() == item, "Incorrect mapped for found element (new)");
        }
    } // end of the thread scope
}
