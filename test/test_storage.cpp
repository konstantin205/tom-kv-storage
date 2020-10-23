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
#include <iostream>
#include <tomkv/storage.hpp>
#include <string>
#include <thread>

namespace pt = boost::property_tree;

// Creates the following tree structure in XML file:
// tom/root
// a {1, 100}
//      b {2, 200}
//      c {3, 300}
//          d {4, 400}
//      e {5, 500}
// b {6, 600}
// f {7, 700}
//      g {8, 800}
std::string prepare_tom( const char* id ) {
    std::string tom_fullname = std::string("tom") + id + ".xml";

    pt::ptree tree;

    tree.add("tom.root.a.key", 1);
    tree.add("tom.root.a.mapped", 100);

    tree.add("tom.root.a.b.key", 2);
    tree.add("tom.root.a.b.mapped", 200);

    tree.add("tom.root.a.c.key", 3);
    tree.add("tom.root.a.c.mapped", 300);

    tree.add("tom.root.a.c.d.key", 4);
    tree.add("tom.root.a.c.d.mapped", 400);

    tree.add("tom.root.a.e.key", 5);
    tree.add("tom.root.a.e.mapped", 500);

    tree.add("tom.root.b.key", 6);
    tree.add("tom.root.b.mapped", 600);

    tree.add("tom.root.f.key", 7);
    tree.add("tom.root.f.key", 700);

    tree.add("tom.root.f.g.key", 8);
    tree.add("tom.root.f.g.mapped", 800);

    pt::write_xml(tom_fullname, tree);
    return tom_fullname;
}

TEST_CASE("test mount and read(single mount)") {
    auto tom_name = prepare_tom("1");
    tomkv::storage<int, int> storage;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    storage.mount(mount_path, tom_name, real_path);

    // Test read key
    auto key_list = storage.key(mount_path + "/d");

    REQUIRE_MESSAGE(key_list.size() == 1, "Only one path should be mounted");
    REQUIRE_MESSAGE(key_list.front() == 4, "Incorrect key on the path mnt/d");

    auto mapped_list = storage.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped_list.size() == 1, "Only one path should be mounted");
    REQUIRE_MESSAGE(mapped_list.front() == 400, "Incorrect mapped on the path mnt/d");

    auto value_list = storage.value(mount_path + "/d");

    REQUIRE_MESSAGE(value_list.size() == 1, "Only one path should be mounted");
    REQUIRE_MESSAGE(value_list.front().first == key_list.front(), "Incorrect key in value on the path mnt/d");
    REQUIRE_MESSAGE(value_list.front().second == mapped_list.front(), "Incorrect mapped in value on the path mnt/d");
}

TEST_CASE("test mount, modify and read (single mount)") {
    auto tom_name = prepare_tom("1");
    tomkv::storage<int, int> storage;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    storage.mount(mount_path, tom_name, real_path);

    // Modify the key
    auto count = storage.set_key(mount_path + "/d", 42);
    REQUIRE_MESSAGE(count == 1, "Only one key should be mounted and modified");

    auto key_list = storage.key(mount_path + "/d");
    REQUIRE_MESSAGE(key_list.size() == 1, "Only one key should be mounted");
    REQUIRE_MESSAGE(key_list.front() == 42, "Key was not modified");

    auto mapped_list = storage.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.front() == 400, "Mapped should not be modified");

    // Modify the mapped
    count = storage.set_mapped(mount_path + "/d", 4200);
    REQUIRE_MESSAGE(count == 1, "Only one mapped should be mounted and modified");

    key_list = storage.key(mount_path + "/d");
    REQUIRE_MESSAGE(key_list.front() == 42, "Key should not be modified");

    mapped_list = storage.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.size() == 1, "Only one mapped should be mounted");
    REQUIRE_MESSAGE(mapped_list.front() == 4200, "Mapped was not modified");

    // Modify the value
    count = storage.set_value(mount_path + "/d", std::pair{22, 2200});
    REQUIRE_MESSAGE(count == 1, "Only one value should be mounted and modified");

    key_list = storage.key(mount_path + "/d");
    REQUIRE_MESSAGE(key_list.size() == 1, "Only one key should be mounted");
    REQUIRE_MESSAGE(key_list.front() == 22, "Key was not modified(value)");

    mapped_list = storage.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.size() == 1, "Only one key should be mounted");
    REQUIRE_MESSAGE(mapped_list.front() == 2200, "Mapped was not modified(value)");
}

TEST_CASE("test unmounted path") {
    tomkv::storage<int, int> st;

    try {
        auto key_list = st.key("a/b/c");
    } catch(tomkv::unmounted_path) {}

    try {
        auto mapped_list = st.mapped("a/b/c");
    } catch(tomkv::unmounted_path) {}

    try {
        auto value_list = st.value("a/b/c");
    } catch(tomkv::unmounted_path) {}
}

TEST_CASE("test mount and read(multiple mount)") {
    auto tom1_name = prepare_tom("1");
    auto tom2_name = prepare_tom("2");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom1_name, real_path);
    st.mount(mount_path, tom2_name, real_path);

    // Test read key
    auto key_list = st.key(mount_path + "/d");

    REQUIRE_MESSAGE(key_list.size() == 2, "Two paths should be mounted");
    for (auto item : key_list) {
        REQUIRE_MESSAGE(item == 4, "Incorrect key in case of multiple mount");
    }

    auto mapped_list = st.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped_list.size() == 2, "Two paths should be mounted");
    for (auto item : mapped_list) {
        REQUIRE_MESSAGE(item == 400, "Incorrect mapped in case of multiple mount");
    }

    auto value_list = st.value(mount_path + "/d");

    REQUIRE_MESSAGE(value_list.size() == 2, "Two paths should be mounted");
    for (auto& item : value_list) {
        REQUIRE_MESSAGE(item.first == 4, "Incorrect key in value in case of multiple mount");
        REQUIRE_MESSAGE(item.second == 400, "Incorrect mapped in value in case of multiple mount");
    }

    tomkv::storage<int, int> st2;
    std::string real_path2 = "f";

    st2.mount(mount_path, tom1_name, real_path);
    st2.mount(mount_path, tom1_name, real_path2); // The path f/d is not exists

    key_list = st2.key(mount_path + "/d");

    REQUIRE_MESSAGE(key_list.size() == 1, "The path is valid for only one mount");
    REQUIRE_MESSAGE(key_list.front() == 4, "Incorrect key in case of multiple mount with one valid path");

    mapped_list = st2.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped_list.size() == 1, "The path is valid for only one mount");
    REQUIRE_MESSAGE(mapped_list.front() == 400, "Incorrect mount in case of multiple mount with one valid path");

    value_list = st2.value(mount_path + "/d");

    REQUIRE_MESSAGE(value_list.size() == 1, "The path is valid for only one mount");
    REQUIRE_MESSAGE(value_list.front().first == key_list.front(), "Incorrect key in value in case of multiple mount with one valid path");
    REQUIRE_MESSAGE(value_list.front().second == mapped_list.front(), "Incorrect mapped in value in case of multiple mount with one valid path");
}

TEST_CASE("test mount, modify and read(multiple mount)") {
    auto tom1_name = prepare_tom("1");
    auto tom2_name = prepare_tom("2");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom1_name, real_path);
    st.mount(mount_path, tom2_name, real_path);

    // Modify the key
    auto count = st.set_key(mount_path + "/d", 42);
    REQUIRE_MESSAGE(count == 2, "One of the keys was not modified");

    auto key_list = st.key(mount_path + "/d");
    REQUIRE_MESSAGE(key_list.size() == 2, "Two keys should be mount");
    for (auto item : key_list) {
        REQUIRE_MESSAGE(item == 42, "One of the keys was not modified");
    }

    auto mapped_list = st.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.size() == 2, "Two mapped should be mount");
    for (auto item : mapped_list) {
        REQUIRE_MESSAGE(item == 400, "Mapped should not be modified");
    }

    // Modify the mapped
    count = st.set_mapped(mount_path + "/d", 4200);
    REQUIRE_MESSAGE(count == 2, "One of the mapped objects was not modified");

    key_list = st.key(mount_path + "/d");
    REQUIRE_MESSAGE(key_list.size() == 2, "Two keys should be mount");
    for (auto item : key_list) {
        REQUIRE_MESSAGE(item == 42, "Key should not be modified");
    }

    mapped_list = st.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.size() == 2, "Two mapped objects should be mount");
    for (auto item : mapped_list) {
        REQUIRE_MESSAGE(item == 4200, "One of the mapped objects was not modified");
    }

    // Modify the value
    count = st.set_value(mount_path + "/d", std::pair{22, 2200});
    REQUIRE_MESSAGE(count == 2, "One of the values was not modified");

    auto value_list = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(value_list.size() == 2, "Two values should be mount");
    for (auto& item : value_list) {
        REQUIRE_MESSAGE(item.first == 22, "Key in value was not modified");
        REQUIRE_MESSAGE(item.second == 2200, "Mapped in value was not modified");
    }

    tomkv::storage<int, int> st2;
    std::string real_path2 = "f";

    st2.mount(mount_path, tom1_name, real_path);
    st2.mount(mount_path, tom1_name, real_path2); // The path f/d is not exists

    // Modify the key
    count = st2.set_key(mount_path + "/d", 48);
    REQUIRE_MESSAGE(count == 1, "Only one key should be modified");

    key_list = st2.key(mount_path + "/d");
    REQUIRE_MESSAGE(key_list.front() == 48, "Key was not modified");

    // Modify the mapped
    count = st2.set_mapped(mount_path + "/d", 4800);
    REQUIRE_MESSAGE(count == 1, "Only one mapped should be modified");

    mapped_list = st2.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.front() == 4800, "Mapped was not modified");

    // Modify the value
    count = st2.set_value(mount_path + "/d", std::pair{55, 5500});
    REQUIRE_MESSAGE(count == 1, "Only one value should be modified");

    value_list = st2.value(mount_path + "/d");
    REQUIRE_MESSAGE(value_list.front().first == 55, "Value was not modified");
    REQUIRE_MESSAGE(value_list.front().second == 5500, "Value was not modified");
}

TEST_CASE("test unmount") {
    auto tom_name = prepare_tom("1");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom_name, real_path);

    // Unmount the mount_path
    bool unmounted = st.unmount(mount_path);
    REQUIRE_MESSAGE(unmounted, "At least one mount should be unmounted");

    // Try to access the mounted node
    try {
        st.key(mount_path + "/d");
        REQUIRE_MESSAGE(false, "st.key call should throw");
    } catch( tomkv::unmounted_path ) {}


    try {
        st.mapped(mount_path + "/d");
        REQUIRE_MESSAGE(false, "st.mapped call should throw");
    } catch( tomkv::unmounted_path ) {}

    try {
        st.value(mount_path + "/d");
        REQUIRE_MESSAGE(false, "st.value call should throw");
    } catch( tomkv::unmounted_path ) {}

    // Try to unmount again
    unmounted = st.unmount(mount_path);
    REQUIRE_MESSAGE(!unmounted, "The second unmount should fail");
}

TEST_CASE("test insert") {
    auto tom_name = prepare_tom("1");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom_name, real_path);

    bool inserted = st.insert(mount_path + "/q", std::pair{42, 4200});
    REQUIRE_MESSAGE(inserted, "Insertion should be successful");

    // Try to read from inserted variable
    auto value_list = st.value(mount_path + "/q");

    REQUIRE_MESSAGE(value_list.size() == 1, "Value should be readed");
    REQUIRE_MESSAGE(value_list.front().first == 42, "Incorrect value inserted");
    REQUIRE_MESSAGE(value_list.front().second == 4200, "Incorrect value inserted");

    // Try to insert once again
    inserted = st.insert(mount_path + "/q", std::pair{22, 2200});
    REQUIRE_MESSAGE(!inserted, "Second insertion should fail");
}

TEST_CASE("test remove") {
    auto tom_name = prepare_tom("1");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom_name, real_path);

    // Erase by valid path
    bool erased = st.remove(mount_path + "/d");
    REQUIRE_MESSAGE(erased, "Element should be erased");

    // Try to read from removed variable
    auto value_list = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(value_list.empty(), "Value was not removed");

    // Erase by invalid path
    erased = st.remove(mount_path + "/d");
    REQUIRE_MESSAGE(!erased, "Second erasure should fail");
}

TEST_CASE("test get mounts") {
    auto tom1_name = prepare_tom("1");
    auto tom2_name = prepare_tom("2");
    auto tom3_name = prepare_tom("3");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom1_name, real_path);
    st.mount(mount_path, tom2_name, real_path);
    st.mount(mount_path, tom3_name, real_path);

    auto mounts = st.get_mounts(mount_path);
    REQUIRE_MESSAGE(mounts.size() == 3, "Three items should be mounted");

    std::vector<std::size_t> found_toms(3, 0);

    for(auto& p : mounts) {
        REQUIRE_MESSAGE(p.second == real_path, "Incorrect mount");
        if (p.first == tom1_name) {
            ++found_toms[0];
        } else if (p.first == tom2_name) {
            ++found_toms[1];
        } else if (p.first == tom3_name) {
            ++found_toms[2];
        } else {
            REQUIRE_MESSAGE(false, "p.first should be one of three toms");
        }
    }

    for(auto item : found_toms) {
        REQUIRE_MESSAGE(item == 1, "All toms should be found");
    }
}

TEST_CASE("test parallel mount") {
    std::vector<std::thread> thread_pool;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    tomkv::storage<int, int> st;

    std::vector<std::string> tom_names(std::thread::hardware_concurrency());

    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        tom_names[i] = prepare_tom(std::to_string(i).c_str());
    }

    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        thread_pool.emplace_back([i, &st, &mount_path, &real_path, &tom_names]{
            st.mount(mount_path, tom_names[i], real_path);
        });
    }

    for (auto& thr : thread_pool) {
        thr.join();
    }

    auto mounts = st.get_mounts(mount_path);
    REQUIRE_MESSAGE(mounts.size() == std::thread::hardware_concurrency(), "Incorrect number of mounts");

    for (auto& mount : mounts) {
        REQUIRE_MESSAGE(mount.second == real_path, "Incorrect mount real path");
        for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            std::string tom_name = "tom";
            tom_name += std::to_string(i);
            tom_name += ".xml";

            auto it = std::find(mounts.cbegin(), mounts.cend(), std::pair{tom_name, real_path});
            REQUIRE_MESSAGE(it != mounts.end(), "Cannot found mount tom");
        }
    }
}

TEST_CASE("test parallel mount-unmount") {
    std::vector<std::thread> thread_pool;

    tomkv::storage<int, int> st;

    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        st.mount(std::string("mnt") + std::to_string(i), "tom.xml", "a/b/c");
    }

    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        thread_pool.emplace_back([i, &st] {
            if (i % 2 == 0) {
                st.mount(std::string("mnt") + std::to_string(i + std::thread::hardware_concurrency()), "tom.xml", "a/b/c");
            } else {
                st.unmount(std::string("mnt") + std::to_string(i));
            }
        });
    }

    for (auto& thr : thread_pool) {
        thr.join();
    }

    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        if (i % 2 == 0) {
            auto mounts = st.get_mounts(std::string("mnt") + std::to_string(i + std::thread::hardware_concurrency()));
            auto mounts2 = st.get_mounts(std::string("mnt") + std::to_string(i));
            REQUIRE_MESSAGE(mounts.size() == 1, "Incorrect size of the mounts");
            REQUIRE_MESSAGE(mounts2.size() == 1, "Even indexes should not be unmounted");
            REQUIRE_MESSAGE(mounts.front().first == "tom.xml", "Incorrect name of the mount tom");
            REQUIRE_MESSAGE(mounts2.front().first == "tom.xml", "Incorrect name of the mount tom");
            REQUIRE_MESSAGE(mounts.front().second == "a/b/c", "Incorrect path of the mount tom");
            REQUIRE_MESSAGE(mounts2.front().second == "a/b/c", "Incorrect path of the mount tom");
        } else {
            auto mounts = st.get_mounts(std::string("mnt") + std::to_string(i));
            REQUIRE_MESSAGE(mounts.size() == 0, "Odd indexes should be unmounted");
        }
    }
}
