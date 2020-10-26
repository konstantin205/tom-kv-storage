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
// j {9, 900}
//      d {10, 1000}
std::string prepare_tom( const char* id, int d_mapped = 400 ) {
    std::string tom_fullname = std::string("tom") + id + ".xml";

    pt::ptree tree;

    tree.add("tom.root.a.key", 1);
    tree.add("tom.root.a.mapped", 100);

    tree.add("tom.root.a.b.key", 2);
    tree.add("tom.root.a.b.mapped", 200);

    tree.add("tom.root.a.c.key", 3);
    tree.add("tom.root.a.c.mapped", 300);

    tree.add("tom.root.a.c.d.key", 4);
    tree.add("tom.root.a.c.d.mapped", d_mapped);

    tree.add("tom.root.a.e.key", 5);
    tree.add("tom.root.a.e.mapped", 500);

    tree.add("tom.root.b.key", 6);
    tree.add("tom.root.b.mapped", 600);

    tree.add("tom.root.f.key", 7);
    tree.add("tom.root.f.mapped", 700);

    tree.add("tom.root.f.g.key", 8);
    tree.add("tom.root.f.g.mapped", 800);

    tree.add("tom.root.j.key", 9);
    tree.add("tom.root.j.mapped", 900);

    tree.add("tom.root.j.d.key", 10);
    tree.add("tom.root.j.d.mapped", 1000);

    pt::write_xml(tom_fullname, tree);
    return tom_fullname;
}

void set_outdated( const std::string& tom_name, std::string path,
                   const std::chrono::seconds& duration )
{
    std::string full_path = std::string("tom.root.") + path;
    pt::ptree tree;

    pt::read_xml(tom_name, tree);

    auto date_created = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto lifetime = duration.count();

    auto opt = tree.get_optional<int>(full_path + ".key");
    REQUIRE(opt);

    tree.put(full_path + ".date_created", date_created);
    tree.put(full_path + ".lifetime", lifetime);

    pt::write_xml(tom_name, tree);
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
    REQUIRE_MESSAGE(*key_list.begin() == 4, "Incorrect key on the path mnt/d");

    auto mapped_list = storage.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped_list.size() == 1, "Only one path should be mounted");
    REQUIRE_MESSAGE(*mapped_list.begin() == 400, "Incorrect mapped on the path mnt/d");

    auto value_list = storage.value(mount_path + "/d");

    REQUIRE_MESSAGE(value_list.size() == 1, "Only one path should be mounted");
    REQUIRE_MESSAGE(value_list.begin()->first == *key_list.begin(), "Incorrect key in value on the path mnt/d");
    REQUIRE_MESSAGE(value_list.begin()->second == *mapped_list.begin(), "Incorrect mapped in value on the path mnt/d");
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
    REQUIRE_MESSAGE(*key_list.begin() == 42, "Key was not modified");

    auto mapped_list = storage.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(*mapped_list.begin() == 400, "Mapped should not be modified");

    // Modify the mapped
    count = storage.set_mapped(mount_path + "/d", 4200);
    REQUIRE_MESSAGE(count == 1, "Only one mapped should be mounted and modified");

    key_list = storage.key(mount_path + "/d");
    REQUIRE_MESSAGE(*key_list.begin() == 42, "Key should not be modified");

    mapped_list = storage.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.size() == 1, "Only one mapped should be mounted");
    REQUIRE_MESSAGE(*mapped_list.begin() == 4200, "Mapped was not modified");

    // Modify the value
    count = storage.set_value(mount_path + "/d", std::pair{22, 2200});
    REQUIRE_MESSAGE(count == 1, "Only one value should be mounted and modified");

    key_list = storage.key(mount_path + "/d");
    REQUIRE_MESSAGE(key_list.size() == 1, "Only one key should be mounted");
    REQUIRE_MESSAGE(*key_list.begin() == 22, "Key was not modified(value)");

    mapped_list = storage.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped_list.size() == 1, "Only one key should be mounted");
    REQUIRE_MESSAGE(*mapped_list.begin() == 2200, "Mapped was not modified(value)");
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
    REQUIRE_MESSAGE(*key_list.begin() == 4, "Incorrect key in case of multiple mount with one valid path");

    mapped_list = st2.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped_list.size() == 1, "The path is valid for only one mount");
    REQUIRE_MESSAGE(*mapped_list.begin() == 400, "Incorrect mount in case of multiple mount with one valid path");

    value_list = st2.value(mount_path + "/d");

    REQUIRE_MESSAGE(value_list.size() == 1, "The path is valid for only one mount");
    REQUIRE_MESSAGE(value_list.begin()->first == *key_list.begin(), "Incorrect key in value in case of multiple mount with one valid path");
    REQUIRE_MESSAGE(value_list.begin()->second == *mapped_list.begin(), "Incorrect mapped in value in case of multiple mount with one valid path");
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
    REQUIRE_MESSAGE(*key_list.begin() == 48, "Key was not modified");

    // Modify the mapped
    count = st2.set_mapped(mount_path + "/d", 4800);
    REQUIRE_MESSAGE(count == 1, "Only one mapped should be modified");

    mapped_list = st2.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(*mapped_list.begin() == 4800, "Mapped was not modified");

    // Modify the value
    count = st2.set_value(mount_path + "/d", std::pair{55, 5500});
    REQUIRE_MESSAGE(count == 1, "Only one value should be modified");

    value_list = st2.value(mount_path + "/d");
    REQUIRE_MESSAGE(value_list.begin()->first == 55, "Value was not modified");
    REQUIRE_MESSAGE(value_list.begin()->second == 5500, "Value was not modified");
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
    REQUIRE_MESSAGE(value_list.begin()->first == 42, "Incorrect value inserted");
    REQUIRE_MESSAGE(value_list.begin()->second == 4200, "Incorrect value inserted");

    // Try to insert once again
    inserted = st.insert(mount_path + "/q", std::pair{22, 2200});
    REQUIRE_MESSAGE(!inserted, "Second insertion should fail");

    // Try to insert key with lifetime
    inserted = st.insert(mount_path + "/qq", std::pair{22, 2200}, std::chrono::seconds(2));
    REQUIRE_MESSAGE(inserted, "Insertion with lifetime should be succeeded");

    // Try to read from inserted variable
    value_list = st.value(mount_path + "/qq");

    REQUIRE_MESSAGE(value_list.size() == 1, "Value should be readed");
    REQUIRE_MESSAGE(value_list.begin()->first == 22, "Incorrect value inserted");
    REQUIRE_MESSAGE(value_list.begin()->second == 2200, "Incorrect value inserted");

    // Try to insert once again
    inserted = st.insert(mount_path + "/qq", std::pair{1, 100});
    REQUIRE_MESSAGE(!inserted, "Key should already exists");
    inserted = st.insert(mount_path + "/qq", std::pair{1, 100}, std::chrono::seconds(100));
    REQUIRE_MESSAGE(!inserted, "Key should already exists");

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // K-V pair is outdated - insertion should be successfull
    inserted = st.insert(mount_path + "/qq", std::pair{33, 3300}, std::chrono::seconds(1));

    value_list = st.value(mount_path + "/qq");
    REQUIRE_MESSAGE(value_list.size() == 1, "Value should be readed");
    REQUIRE_MESSAGE(value_list.begin()->first == 33, "Incorrect value inserted");
    REQUIRE_MESSAGE(value_list.begin()->second == 3300, "Incorrect value inserted");

    inserted = st.insert(mount_path + "/qq", std::pair{11, 1100});
    REQUIRE_MESSAGE(!inserted, "Key should already exists");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    inserted = st.insert(mount_path + "/qq", std::pair{48, 4800});
    REQUIRE_MESSAGE(inserted, "Value should be inserted");

    value_list = st.value(mount_path + "/qq");
    REQUIRE_MESSAGE(value_list.size() == 1, "Value should be readed");
    REQUIRE_MESSAGE(value_list.begin()->first == 48, "Incorrect value inserted");
    REQUIRE_MESSAGE(value_list.begin()->second == 4800, "Incorrect value inserted");
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

    bool inserted = st.insert(mount_path + "/d", std::pair{100, 1000}, std::chrono::seconds(1));
    REQUIRE_MESSAGE(inserted, "Insertion should be succeeded");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    erased = st.remove(mount_path + "/d");
    REQUIRE_MESSAGE(!erased, "Erasure of outdated key should fail");
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

TEST_CASE("test mount with priority") {
    auto tom1_name = prepare_tom("1", 42);
    auto tom2_name = prepare_tom("2", 4242);
    auto tom3_name = prepare_tom("3", 4242);

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    tomkv::storage<int, int> st;

    st.mount(mount_path, tom1_name, real_path, /*priority = */1);
    st.mount(mount_path, tom2_name, real_path, /*priority = */2);
    st.mount(mount_path, tom3_name, real_path); // Lowest priority by default

    auto key_mset = st.key(mount_path + "/d");

    REQUIRE_MESSAGE(key_mset.size() == 1, "Storage should read only the high-priority key");
    REQUIRE_MESSAGE(*key_mset.begin() == 4, "Incorrect key returned from priority reading");

    auto mapped_mset = st.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped_mset.size() == 1, "Storage should read only one mapped from high-priority key");
    REQUIRE_MESSAGE(*mapped_mset.begin() == 4242, "Incorrect mapped returned from priority reading");

    auto value_mmap = st.value(mount_path + "/d");

    REQUIRE_MESSAGE(value_mmap.size() == 1, "Storage should read only one value from high-priority key");
    REQUIRE_MESSAGE(value_mmap.begin()->first == 4, "Incorrect key (from value) returned from priority reading");
    REQUIRE_MESSAGE(value_mmap.begin()->second == 4242, "Incorrect mapped (from value) returned from priority reading");

    // Mount one more path in tom without priority
    st.mount(mount_path, tom1_name, "j");

    key_mset = st.key(mount_path + "/d");

    REQUIRE_MESSAGE(key_mset.size() == 2, "Storage should read two values - one with high priority and an other without priority");
    auto it = key_mset.find(4); // High priority key
    REQUIRE_MESSAGE(it != key_mset.end(), "High priority key should be found");
    REQUIRE_MESSAGE(*it == 4, "Incorrect iterator returned from find");
    it = key_mset.find(10); // Key with no priority
    REQUIRE_MESSAGE(it != key_mset.end(), "Key with no priority should be found");
    REQUIRE_MESSAGE(*it == 10, "Incorrect iterator returned from find");

    mapped_mset = st.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped_mset.size() == 2, "Storage should read two values - one with priority and an other without priority");
    auto it2 = mapped_mset.find(4242); // Mapped from priority key
    REQUIRE_MESSAGE(it2 != mapped_mset.end(), "Mapped from high priority key should be found");
    REQUIRE_MESSAGE(*it2 == 4242, "Incorrect iterator returned from find");
    it2 = mapped_mset.find(1000); // Mapped from key with no priority
    REQUIRE_MESSAGE(it2 != mapped_mset.end(), "Mapped from key with no priority should be found");
    REQUIRE_MESSAGE(*it2 == 1000, "Incorrect iterator returned from find");

    value_mmap = st.value(mount_path + "/d");

    REQUIRE_MESSAGE(value_mmap.size() == 2, "Storage should read two values - one with priority and an other without priority");
    auto it3 = value_mmap.find(4); // High priority key
    REQUIRE_MESSAGE(it3 != value_mmap.end(), "High priority key should be found");
    REQUIRE_MESSAGE(it3->first == 4, "Incorrect iterator returned from find");
    REQUIRE_MESSAGE(it3->second == 4242, "Mapped from the key with high priority should be returned");
    it3 = value_mmap.find(10); // Key with no priority
    REQUIRE_MESSAGE(it3 != value_mmap.end(), "Key with no priority should be found");
    REQUIRE_MESSAGE(it3->first == 10, "Incorrect iterator returned from find");
    REQUIRE_MESSAGE(it3->second == 1000, "Mapped from the key with no priority should be returned");
}

TEST_CASE("test modify key/mapped/value") {
    auto tom_name = prepare_tom("1");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom_name, real_path);

    auto key = *st.key(mount_path + "/d").begin();
    auto mapped = *st.mapped(mount_path + "/d").begin();

    // Modify key
    std::size_t modified = st.modify_key(mount_path + "/d", [](int i) { return i + 1; } );

    REQUIRE_MESSAGE(modified == 1, "One key should be modified");

    auto key_after_modification = *st.key(mount_path + "/d").begin();
    auto mapped_after_modification = *st.mapped(mount_path + "/d").begin();

    REQUIRE_MESSAGE(key_after_modification == key + 1, "Incorrect key after key modification");
    REQUIRE_MESSAGE(mapped_after_modification == mapped, "Mapped should not be changed");

    key = key_after_modification;

    // Modify mapped
    modified = st.modify_mapped(mount_path + "/d", [](int m) { return m + 1; });

    REQUIRE_MESSAGE(modified == 1, "One mapped should be modified");

    key_after_modification = *st.key(mount_path + "/d").begin();
    mapped_after_modification = *st.mapped(mount_path + "/d").begin();

    REQUIRE_MESSAGE(key_after_modification == key, "Key should not be modified");
    REQUIRE_MESSAGE(mapped_after_modification == mapped + 1, "Incorrect mapped after modification");

    mapped = mapped_after_modification;

    // Modify value
    modified = st.modify_value(mount_path + "/d", [](auto v) { return std::pair{v.first + 1, v.second + 1}; });

    REQUIRE_MESSAGE(modified == 1, "One value should be modified");

    key_after_modification = *st.key(mount_path + "/d").begin();
    mapped_after_modification = *st.mapped(mount_path + "/d").begin();

    REQUIRE_MESSAGE(key_after_modification == key + 1, "Incorrect key after modification");
    REQUIRE_MESSAGE(mapped_after_modification == mapped + 1, "Incorrect mapped after modification");

    auto value = *st.value(mount_path + "/d").begin();

    REQUIRE_MESSAGE(value.first == key + 1, "Incorrect key(from value) after modification");
    REQUIRE_MESSAGE(value.second == mapped + 1, "Incorrect mapped(from value) after modification");
}

TEST_CASE("test read outdated keys") {
    auto tom_name = prepare_tom("1");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom_name, real_path);

    set_outdated(tom_name, "a.c.d", std::chrono::seconds(2));

    // Try to read - should not be outdated
    auto keys = st.key(mount_path + "/d");

    REQUIRE_MESSAGE(keys.size() == 1, "Only one element should be found");
    REQUIRE_MESSAGE(*keys.begin() == 4, "Incorrect key");

    auto mapped = st.mapped(mount_path + "/d");

    REQUIRE_MESSAGE(mapped.size() == 1, "Only one element should be found");
    REQUIRE_MESSAGE(*mapped.begin() == 400, "Incorrect mapped");

    auto values = st.value(mount_path + "/d");

    REQUIRE_MESSAGE(values.size() == 1, "Only one element should be found");
    REQUIRE_MESSAGE(values.begin()->first == 4, "Incorrect key(value)");
    REQUIRE_MESSAGE(values.begin()->second == 400, "Incorrect mapped(value)");

    // Make the key outdated
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Key should not be available any more
    keys = st.key(mount_path + "/d");
    REQUIRE_MESSAGE(keys.size() == 0, "Key should not be available");

    mapped = st.mapped(mount_path + "/d");
    REQUIRE_MESSAGE(mapped.size() == 0, "Mapped should not be available");

    values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 0, "Value should not be available");
}

TEST_CASE("test write outdated keys") {
    auto tom_name = prepare_tom("1");

    tomkv::storage<int, int> st;

    std::string mount_path = "mnt";
    std::string real_path = "a/c";

    st.mount(mount_path, tom_name, real_path);

    set_outdated(tom_name, "a.c.d", std::chrono::seconds(1));

    // Try to read - should not be outdated
    auto values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 1, "Only one element should be found");
    REQUIRE_MESSAGE(values.begin()->first == 4, "Incorrect key");
    REQUIRE_MESSAGE(values.begin()->second == 400, "Incorrect mapped");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // K-V pair should not be available for modification
    std::size_t modified = st.set_key(mount_path + "/d", 42);
    REQUIRE_MESSAGE(modified == 0, "No key should be modified");

    modified = st.set_mapped(mount_path + "/d", 4242);
    REQUIRE_MESSAGE(modified == 0, "No mapped should be modified");

    modified = st.set_value(mount_path + "/d", std::pair{42, 4242});
    REQUIRE_MESSAGE(modified == 0, "No value should be modified");

    // Test modify as new
    modified = st.set_key_as_new(mount_path + "/d", 42);
    REQUIRE_MESSAGE(modified == 1, "One key should be modified");

    // Reading should be successful
    values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 1, "One key should be found");
    REQUIRE_MESSAGE(values.begin()->first == 42, "Incorrect key");
    REQUIRE_MESSAGE(values.begin()->second == 400, "Incorrect mapped");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // K-V pair should not be available any more
    values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 0, "No keys should be found");

    // Modify mapped as new
    modified = st.set_mapped_as_new(mount_path + "/d", 4242);
    REQUIRE_MESSAGE(modified == 1, "One element should be modified");

    // Reading should be successful
    values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 1, "One key should be found");
    REQUIRE_MESSAGE(values.begin()->first == 42, "Incorrect key");
    REQUIRE_MESSAGE(values.begin()->second == 4242, "Incorrect mapped");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // K-V pair should not be available any more
    values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 0, "No keys should be found");

    // Modify value as new
    modified = st.set_value_as_new(mount_path + "/d", std::pair{22, 2200});
    REQUIRE_MESSAGE(modified == 1, "One element should be modified");

    // Reading should be successful
    values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 1, "One key should be found");
    REQUIRE_MESSAGE(values.begin()->first == 22, "Incorrect key");
    REQUIRE_MESSAGE(values.begin()->second == 2200, "Incorrect mapped");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // K-V pair should not be available any more
    values = st.value(mount_path + "/d");
    REQUIRE_MESSAGE(values.size() == 0, "No keys should be found");
}
