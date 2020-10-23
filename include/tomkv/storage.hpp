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

#ifndef __TOMKV_INCLUDE_STORAGE_HPP
#define __TOMKV_INCLUDE_STORAGE_HPP

#include "internal/hash_table.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/property_tree/exceptions.hpp"
#include <string>
#include <list>
#include <functional>
#include <utility>
#include <atomic>
#include <memory>
#include <mutex>
#include <algorithm>
#include <tuple>

namespace tomkv {
namespace internal {

namespace ptree = boost::property_tree;

// TODO: specify what() for exception types
// The type of the exception which is thrown if the path passed to the lookup/erase/insert is unmounted
struct unmounted_path : std::exception {};

template <typename Key, typename Mapped>
class storage {
public:
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<key_type, mapped_type>;

    using node_id = std::string;
    using mount_id = std::string;
    using tom_id = std::string;
    using path_type = std::string;
private:
    static_assert(std::is_same_v<mount_id, tom_id>);
    using id_hasher = std::hash<mount_id>;
    using id_equality = std::equal_to<mount_id>;

    using allocator_type = std::allocator<value_type>; // TODO: change allocator template
    using allocator_traits_type = std::allocator_traits<allocator_type>;
public:
    storage() : my_mount_table(my_hasher, my_id_equality, my_allocator),
                my_tom_table(my_hasher, my_id_equality, my_allocator) {}

    storage( const storage& );
    storage( storage&& );

    ~storage() {}

    storage& operator=( const storage& );
    storage& operator=( storage&& );

    void mount( const mount_id& m_id, const tom_id& t_id, const path_type& path ) {
        internal_mount(m_id, t_id, path);
    }

    bool unmount( const mount_id& m_id ) {
        return internal_unmount(m_id);
    }

    std::list<std::pair<tom_id, path_type>> get_mounts( const mount_id& path ) {
        return internal_get_mounts(path);
    }

    std::list<key_type> key( const path_type& path ) {
        return internal_key(path);
    }

    std::list<mapped_type> mapped( const path_type& path ) {
        return internal_mapped(path);
    }

    std::list<value_type> value( const path_type& path ) {
        return internal_value(path);
    }

    std::size_t set_key( const path_type& path, const key_type& key ) {
        return internal_set_key(path, key);
    }

    std::size_t set_mapped( const path_type& path, const mapped_type& mapped ) {
        return internal_set_mapped(path, mapped);
    }

    std::size_t set_value( const path_type& path, const value_type& value ) {
        return internal_set_value(path, value);
    }

    bool insert( const path_type& path, const value_type& value ) {
        return internal_insert(path, value);
    }

    bool remove( const path_type& path ) {
        return internal_remove(path);
    }

private:
    class mount_info {
    public:
        mount_info( const tom_id& tom_name, const path_type& real_path ) noexcept
            : my_tom_name(tom_name), my_real_path(real_path) {}

        const tom_id& tom_name() const { return my_tom_name; }
        const path_type& real_path() const { return my_real_path; }
    private:
        tom_id my_tom_name;
        path_type my_real_path;
    };

    class tom_info {
        using tree_allocator_type = typename allocator_traits_type::template rebind_alloc<ptree::ptree>;
        using tree_allocator_traits = std::allocator_traits<tree_allocator_type>;
    public:
        tom_info( const tom_id& t_id )
            : my_tree(nullptr), my_tom_id(t_id), my_pending_readers(0), my_pending_writers(0) {}

        using lock_type = std::unique_lock<std::mutex>;

        lock_type lock() { return lock_type{my_mutex}; }

        void add_pending_reader() { my_pending_readers.fetch_add(1, std::memory_order_relaxed); }
        void add_pending_writer() { my_pending_writers.fetch_add(1, std::memory_order_relaxed); }

        void remove_pending_reader() { my_pending_readers.fetch_sub(1, std::memory_order_relaxed); }
        void remove_pending_writer() { my_pending_writers.fetch_sub(1, std::memory_order_relaxed); }

        std::size_t pending_readers() const { return my_pending_readers.load(std::memory_order_relaxed); }
        std::size_t pending_writers() const { return my_pending_writers.load(std::memory_order_relaxed); }

        ptree::ptree* tree() const { return my_tree; }

        // Should be called when my_mutex is locked
        void create_tree( tree_allocator_type alloc ) {
            __TOMKV_ASSERT(my_tree == nullptr);
            my_tree = tree_allocator_traits::allocate(alloc, 1);
            tree_allocator_traits::construct(alloc, my_tree);
            ptree::read_xml(my_tom_id, *my_tree);
        }

        // Should be called when my_mutex is locked
        void destroy_tree( tree_allocator_type alloc ) {
            __TOMKV_ASSERT(my_tree != nullptr);
            tree_allocator_traits::destroy(alloc, my_tree);
            tree_allocator_traits::deallocate(alloc, my_tree, 1);
            my_tree = nullptr;
        }

        // Should be called when my_mutex is locked
        void dump_tree() {
            __TOMKV_ASSERT(my_tree != nullptr);
            ptree::write_xml(my_tom_id, *my_tree);
        }

    private:
        std::mutex my_mutex;
        ptree::ptree* my_tree; // Protected by my_mutex
        const tom_id& my_tom_id;
        std::atomic<std::size_t> my_pending_readers;
        std::atomic<std::size_t> my_pending_writers;
    };

    class mount_node {
    public:
        mount_node( mount_info&& info ) noexcept : my_next(nullptr), my_info(std::move(info)) {}

        mount_node* next() const { return my_next; }

        // Not thread-safe
        void set_next( mount_node* n ) { my_next = n; }

        const tom_id& tom_name() const { return my_info.tom_name(); }
        const path_type& real_path() const { return my_info.real_path(); }
    private:
        mount_node* my_next;
        mount_info my_info;
    };

    using mount_hash_table = hash_table<mount_id, std::atomic<mount_node*>,
                                        id_hasher, id_equality, allocator_type>;
    using tom_hash_table = hash_table<tom_id, tom_info, id_hasher,
                                      id_equality, allocator_type>;

    using mount_read_accessor = typename mount_hash_table::read_accessor;
    using mount_write_accessor = typename mount_hash_table::write_accessor;
    using tom_read_accessor = typename tom_hash_table::read_accessor;
    using tom_write_accessor = typename tom_hash_table::write_accessor;

    using mount_node_allocator_type = typename allocator_traits_type::template rebind_alloc<mount_node>;
    using mount_node_allocator_traits = std::allocator_traits<mount_node_allocator_type>;

    using ptree_path_type = typename ptree::ptree::path_type;

    mount_node* create_mount_node( const tom_id& t_id, const path_type& path ) {
        mount_node_allocator_type mount_allocator(my_allocator);

        mount_info new_mount_info( t_id, path );

        mount_node* new_mount_node = mount_node_allocator_traits::allocate(mount_allocator, 1);
        // mount_node constructor is noexcept => no extra care needed
        mount_node_allocator_traits::construct(mount_allocator, new_mount_node, std::move(new_mount_info));

        return new_mount_node;
    }

    void delete_mount_node( mount_node* node ) {
        mount_node_allocator_type mount_allocator(my_allocator);
        mount_node_allocator_traits::destroy(mount_allocator, node);
        mount_node_allocator_traits::deallocate(mount_allocator, node, 1);
    }

    // TODO: priority?
    void internal_mount( const mount_id& m_id, const tom_id& t_id, const path_type& path ) {
        mount_node* new_mount_node = create_mount_node(t_id, path);

        // Add tom into tom table
        my_tom_table.emplace(std::piecewise_construct,
                             std::forward_as_tuple(t_id), // Args for key
                             std::forward_as_tuple(t_id)); // Args for mapped

        mount_read_accessor mracc;
        bool inserted = my_mount_table.emplace(mracc, m_id, new_mount_node);
        if (inserted) {
            // Current thread successfully mounted new mount_id into the mount_table
            // We can just exit here
            return;
        }

        // mount_id was already mounted previously - we need to append new mount_id into the list
        std::atomic<mount_node*>& list = mracc.hazardous_mapped();
        mount_node* expected = list.load(std::memory_order_relaxed);
        new_mount_node->set_next(expected);

        utils::exponential_backoff backoff;

        while(!list.compare_exchange_weak(expected, new_mount_node,
                                          std::memory_order_relaxed,
                                          std::memory_order_relaxed))
        {
            backoff.pause();
        }

        // In this point, new tom and path were successfully inserted into the mount list
        return;
    }

    bool internal_unmount( const mount_id& m_id ) {
        mount_write_accessor mwacc;
        bool found = my_mount_table.find(mwacc, m_id);

        if (found) {
            mount_node* list_node = mwacc.mapped().load(std::memory_order_relaxed);

            while(list_node != nullptr) {
                mount_node* node_to_remove = list_node;
                list_node = list_node->next();
                delete_mount_node(node_to_remove);
            }

            my_mount_table.erase(mwacc);
        }
        return found;
    }

    std::list<std::pair<tom_id, path_type>> internal_get_mounts( const mount_id& m_id ) {
        std::list<std::pair<tom_id, path_type>> mounts;

        mount_read_accessor mracc;
        bool found = my_mount_table.find(mracc, m_id);

        if (found) {
            mount_node* list_node = mracc.mapped().load(std::memory_order_relaxed);

            while(list_node != nullptr) {
                mounts.emplace_back(list_node->tom_name(), list_node->real_path());
                list_node = list_node->next();
            }
        }
        return mounts;
    }

    mount_read_accessor split_and_find_impl( path_type& mount_path, path_type& path ) {
        if (path.empty()) {
            throw unmounted_path{};
        }

        path_type additional_path;
        additional_path.reserve(path.size());

        path_type::const_iterator it;
        for (it = path.cbegin(); it != path.cend(); ++it) {
            if (*it == '/') {
                break;
            }
        }

        // Split path into something before the first delimiter (/)
        // and other characters
        mount_path.append(path.cbegin(), it);
        if (it != path.cend()) {
            additional_path.append(std::next(it), path.cend());
        }

        path = std::move(additional_path);

        mount_read_accessor mracc;

        if (my_mount_table.find(mracc, mount_path)) {
            // Mounting successfully found
            return mracc;
        }
        // Mounting was not found
        mount_path.append("/"); // Add a delimiter
        return split_and_find_impl(mount_path, path);
    }

    mount_read_accessor split_and_find( path_type& path ) {
        path_type mount_path;
        mount_path.reserve(path.size());
        return split_and_find_impl(mount_path, path);
    }

    template <bool IsWriteOperation, typename Body, typename... AdditionalArgs>
    void basic_operation( const path_type& path, const Body& body, AdditionalArgs&&... additional_args ) {
        // Will be passed to split_and_find by reference argument
        // mount path will be cutted from the beginning
        path_type additional_path = path;

        mount_read_accessor mracc = split_and_find(additional_path);

        // Catch the mount list - serialization point
        mount_node* curr_mount_node = mracc.mapped().load(std::memory_order_relaxed);

        while(curr_mount_node != nullptr) {
            tom_read_accessor tracc;
            bool found = my_tom_table.find(tracc, curr_mount_node->tom_name());
            __TOMKV_ASSERT(found);

            tom_info& t_info = tracc.hazardous_mapped();

            if constexpr (IsWriteOperation) {
                // We are write operation
                t_info.add_pending_writer();
            } else {
                // We are read operation
                t_info.add_pending_reader();
            }

            auto lock = t_info.lock();

            // Lock is acquired - remove current thread from the readers/writers

            if constexpr (IsWriteOperation) {
                t_info.remove_pending_writer();
            } else {
                t_info.remove_pending_reader();
            }

            // Read a tree from XML if it was not done already by an other thread
            if (t_info.tree() == nullptr) {
                t_info.create_tree(my_allocator);
            }

            path_type node_path;
            // 9 is the number of characters in "tom/root/"
            // 1 is the delimiter
            node_path.reserve( 9 + curr_mount_node->real_path().size() + additional_path.size() + 1 );

            node_path.append("tom/root/");
            node_path.append(curr_mount_node->real_path());
            node_path.append("/");
            node_path.append(additional_path);

            body(node_path, t_info.tree(), std::forward<AdditionalArgs>(additional_args)...);

            // Operation completed
            if constexpr (IsWriteOperation) {
                if (t_info.pending_writers() == 0) {
                    // If there are no pending write operations - we need to dump tree to XML
                    t_info.dump_tree();
                }
            }

            if (t_info.pending_readers() == 0 && t_info.pending_writers() == 0) {
                // If no pending read/write operations - destroy the tree
                t_info.destroy_tree(my_allocator);
            }
            curr_mount_node = curr_mount_node->next();
        }
    }

    std::list<key_type> internal_key( const path_type& path ) {
        std::list<key_type> key_list;

        auto body = []( const path_type& node_path, ptree::ptree* tree, std::list<key_type>& key_list ) {
            try {
                key_type key_from_tom = tree->template get<key_type>(ptree_path_type{node_path + "/key", '/'});
                key_list.emplace_back(std::move(key_from_tom)); // Will not be called if ptree_bad_path was thrown
            } catch( ptree::ptree_bad_path ) {}
        };

        basic_operation</*Writer?*/false>(path, body, key_list);
        return key_list;
    }

    std::list<mapped_type> internal_mapped( const path_type& path ) {
        std::list<mapped_type> mapped_list;

        auto body = []( const path_type& node_path, ptree::ptree* tree, std::list<mapped_type>& mapped_list ) {
            try {
                mapped_type mapped_from_tom = tree->template get<mapped_type>(ptree_path_type{node_path + "/mapped", '/'});
                mapped_list.emplace_back(std::move(mapped_from_tom)); //  Will not be called if ptree_bad_path was thrown
            } catch( ptree::ptree_bad_path ) {}
        };

        basic_operation</*Writer?*/false>(path, body, mapped_list);
        return mapped_list;
    }

    std::list<value_type> internal_value( const path_type& path ) {
        std::list<value_type> value_list;

        auto body = []( const path_type& node_path, ptree::ptree* tree, std::list<value_type>& value_list ) {
            try {
                key_type key_from_tom = tree->template get<key_type>(ptree_path_type{node_path + "/key", '/'});
                mapped_type mapped_from_tom = tree->template get<mapped_type>(ptree_path_type{node_path + "/mapped", '/'});
                value_list.emplace_back(std::move(key_from_tom), std::move(mapped_from_tom)); //  Will not be called if ptree_bad_path was thrown
            } catch( ptree::ptree_bad_path ) {}
        };

        basic_operation</*Writer*/false>(path, body, value_list);
        return value_list;
    }

    std::size_t internal_set_key( const path_type& path, const key_type& key ) {
        std::size_t modified_keys_counter = 0;

        auto body = [&modified_keys_counter]( const path_type& node_path, ptree::ptree* tree, const key_type& key ) {
            try {
                auto path_to_key = ptree_path_type{node_path + "/key", '/'};
                tree->template get<key_type>(path_to_key); // Throws in case of invalid path TODO: find better alternative
                tree->put(path_to_key, key);
                ++modified_keys_counter;
            } catch ( ptree::ptree_bad_path ) {}
        };

        basic_operation</*Writer?*/true>(path, body, key);

        return modified_keys_counter;
    }

    std::size_t internal_set_mapped( const path_type& path, const mapped_type& mapped ) {
        std::size_t modified_mapped_counter = 0;

        auto body = [&modified_mapped_counter]( const path_type& node_path, ptree::ptree* tree, const mapped_type& mapped ) {
            try {
                auto path_to_mapped = ptree_path_type{node_path + "/mapped", '/'};
                tree->template get<mapped_type>(path_to_mapped); // Throws in case of invalid path TODO: find better alternative
                tree->put(path_to_mapped, mapped);
                ++modified_mapped_counter;
            } catch( ptree::ptree_bad_path ) {}
        };

        basic_operation</*Writer?*/true>(path, body, mapped);

        return modified_mapped_counter;
    }

    std::size_t internal_set_value( const path_type& path, const value_type& value ) {
        std::size_t modified_value_counter = 0;

        auto body = [&modified_value_counter]( const path_type& node_path, ptree::ptree* tree, const value_type& value ) {
            try {
                auto path_to_key = ptree_path_type{node_path + "/key", '/'};
                // We can only check the path validity for key path
                tree->template get<key_type>(path_to_key);
                tree->put(path_to_key, value.first);
                tree->put(ptree_path_type{node_path + "/mapped", '/'}, value.second);
                ++modified_value_counter;
            } catch( ptree::ptree_bad_path ) {}
        };

        basic_operation</*Writer?*/true>(path, body, value);

        return modified_value_counter;
    }

    bool internal_insert( const path_type& path, const value_type& value ) {
        bool exists = false;
        auto body = [&]( const path_type& node_path, ptree::ptree* tree, const value_type& value ) {
            auto path_to_key = ptree_path_type{node_path + "/key", '/'};
            // Check if the path is already in a tree
            try {
                tree->template get<key_type>(path_to_key);
                exists = true;
                return;
            } catch( ptree::ptree_bad_path& ) {
                exists = false;
            }

            tree->put(path_to_key, value.first);
            tree->put(ptree_path_type{node_path + "/mapped", '/'}, value.second);
        };

        basic_operation</*Writer?*/true>(path, body, value);
        return !exists;
    }

    bool internal_remove( const path_type& path ) {
        bool exists = false;
        auto body = [&]( const path_type& node_path, ptree::ptree* tree ) {
            auto path_to_key = ptree_path_type{node_path + "/key", '/'};
            // Check if the path exists
            try {
                tree->template get<key_type>(path_to_key);
                exists = true;
            } catch( ptree::ptree_bad_path& ) {
                exists = false;
                return;
            }

            // Cutting the last element after the delimiter
            path_type::const_iterator it;

            for (it = node_path.cend(); it != node_path.cbegin(); --it) {
                if (*it == '/') break;
            }

            path_type path_to_prev_node(node_path.cbegin(), it);
            path_type curr_node_name(std::next(it), node_path.cend());

            tree->get_child(ptree_path_type{path_to_prev_node, '/'}).erase(curr_node_name);
        };

        basic_operation</*Write?*/true>(path, body);

        return exists;
    }

    allocator_type                      my_allocator;
    id_hasher                           my_hasher;
    id_equality                         my_id_equality;
    mount_hash_table                    my_mount_table;
    tom_hash_table                      my_tom_table;
}; // class storage
} // namespace internal

using internal::storage;
using internal::unmounted_path;

} // namespace tomkv

#endif // __TOMKV_INCLUDE_STORAGE_HPP
