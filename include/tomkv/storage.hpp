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
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <chrono>

namespace tomkv {
namespace internal {

namespace ptree = boost::property_tree;

// The type of the exception which is thrown if the path passed to the lookup/erase/insert is unmounted
struct unmounted_path : std::exception {
    const char* what() const noexcept override {
        return "Mounted path was not found";
    }
}; // struct unmounted_path

template <typename Key, typename Mapped,
          typename Allocator = std::allocator<std::pair<const Key, Mapped>>>
class storage {
public:
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<key_type, mapped_type>;

    using mount_id = std::string;
    using tom_id = std::string;
    using path_type = std::string;
    using priority_type = std::size_t;
private:
    static_assert(std::is_same_v<mount_id, tom_id>);
    using id_hasher = std::hash<mount_id>;
    using id_equality = std::equal_to<mount_id>;

    using allocator_type = Allocator; // TODO: change allocator template
    using allocator_traits_type = std::allocator_traits<allocator_type>;
public:
    storage( const allocator_type& alloc = allocator_type() )
        : my_allocator(alloc),
          my_mount_table(my_hasher, my_id_equality, my_allocator),
          my_tom_table(my_hasher, my_id_equality, my_allocator) {}

    storage( const storage& ) = delete;
    storage& operator=( const storage& ) = delete;

    ~storage() {
        internal_destroy();
    }

    allocator_type get_allocator() const { return my_allocator; }

    void mount( const mount_id& m_id, const tom_id& t_id,
                const path_type& path, priority_type priority = priority_type(0) ) {
        internal_mount(m_id, t_id, path, priority);
    }

    bool unmount( const mount_id& m_id ) {
        return internal_unmount(m_id);
    }

    std::list<std::pair<tom_id, path_type>> get_mounts( const mount_id& path ) {
        return internal_get_mounts(path);
    }

    std::unordered_multiset<key_type> key( const path_type& path ) {
        return internal_key(path);
    }

    std::unordered_multiset<mapped_type> mapped( const path_type& path ) {
        return internal_mapped(path);
    }

    std::unordered_multimap<key_type, mapped_type> value( const path_type& path ) {
        return internal_value(path);
    }

    std::size_t set_key( const path_type& path, const key_type& key ) {
        return modify_key(path, [&key]( const key_type& ) { return key; });
    }

    std::size_t set_mapped( const path_type& path, const mapped_type& mapped ) {
        return modify_mapped(path, [&mapped]( const mapped_type& ) { return mapped; });
    }

    std::size_t set_value( const path_type& path, const value_type& value ) {
        return modify_value(path, [&value]( const value_type& ) { return value; });
    }

    std::size_t set_key_as_new( const path_type& path, const key_type& key ) {
        return modify_key_as_new(path, [&key]( const key_type& ) { return key; });
    }

    std::size_t set_mapped_as_new( const path_type& path, const mapped_type& mapped ) {
        return modify_mapped_as_new(path, [&mapped]( const mapped_type& ) { return mapped; });
    }

    std::size_t set_value_as_new( const path_type& path, const value_type& value ) {
        return modify_value_as_new(path, [&value]( const value_type& ) { return value; });
    }

    template <typename Predicate>
    std::size_t modify_key( const path_type& path, const Predicate& pred ) {
        return internal_modify_key(path, pred);
    }

    template <typename Predicate>
    std::size_t modify_mapped( const path_type& path, const Predicate& pred ) {
        return internal_modify_mapped(path, pred);
    }

    template <typename Predicate>
    std::size_t modify_value( const path_type& path, const Predicate& pred ) {
        return internal_modify_value(path, pred);
    }

    template <typename Predicate>
    std::size_t modify_key_as_new( const path_type& path, const Predicate& pred ) {
        return internal_modify_key_as_new(path, pred);
    }

    template <typename Predicate>
    std::size_t modify_mapped_as_new( const path_type& path, const Predicate& pred ) {
        return internal_modify_mapped_as_new(path, pred);
    }

    template <typename Predicate>
    std::size_t modify_value_as_new( const path_type& path, const Predicate& pred ) {
        return internal_modify_value_as_new(path, pred);
    }

    bool insert( const path_type& path, const value_type& value ) {
        return internal_insert(path, value);
    }

    bool insert( const path_type& path, const value_type& value,
                 const std::chrono::seconds& lifetime )
    {
        return internal_insert_with_lifetime(path, value, lifetime);
    }

    bool remove( const path_type& path ) {
        return internal_remove(path);
    }

private:
    class mount_info {
    public:
        mount_info( const tom_id& tom_name, const path_type& real_path, priority_type priority ) noexcept
            : my_tom_name(tom_name), my_real_path(real_path), my_priority(priority) {}

        const tom_id& tom_name() const { return my_tom_name; }
        const path_type& real_path() const { return my_real_path; }

        priority_type priority() const { return my_priority; }
    private:
        tom_id my_tom_name;
        path_type my_real_path;
        priority_type my_priority;
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

        priority_type priority() const { return my_info.priority(); }
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

    using date_type = std::chrono::seconds::rep;

    mount_node* create_mount_node( const tom_id& t_id, const path_type& path,
                                   priority_type priority ) {
        mount_node_allocator_type mount_allocator(my_allocator);

        mount_info new_mount_info(t_id, path, priority);

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

    void internal_mount( const mount_id& m_id, const tom_id& t_id,
                         const path_type& path, priority_type priority ) {
        mount_node* new_mount_node = create_mount_node(t_id, path, priority);

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
        mount_node* expected = list.load(std::memory_order_acquire);
        new_mount_node->set_next(expected);

        utils::exponential_backoff backoff;

        while(!list.compare_exchange_weak(expected, new_mount_node,
                                          std::memory_order_relaxed,
                                          std::memory_order_relaxed))
        {
            new_mount_node->set_next(expected);
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
            utils::suppress_unused(found);

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
            if (!additional_path.empty()) {
                node_path.append("/");
                node_path.append(additional_path);
            }

            body(node_path, t_info.tree(), curr_mount_node->priority(), std::forward<AdditionalArgs>(additional_args)...);

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

    std::unordered_multiset<key_type> internal_key( const path_type& path ) {
        std::unordered_multimap<key_type, priority_type> keys_with_priority;

        auto body = []( const path_type& node_path, ptree::ptree* tree, priority_type current_priority,
                        std::unordered_multimap<key_type, priority_type>& key_priority_map ) {
            try {
                auto date_created = tree->template get_optional<date_type>(ptree_path_type{node_path + "/date_created", '/'});
                auto lifetime = tree->template get_optional<date_type>(ptree_path_type{node_path + "/lifetime", '/'});

                bool is_outdated = false;

                // If the lifetime for the key is presented
                if (date_created && lifetime) {
                    is_outdated = (std::chrono::system_clock::now() - std::chrono::seconds(lifetime.value())) >
                                  std::chrono::system_clock::time_point(std::chrono::seconds(date_created.value()));
                }

                if (!is_outdated) {
                    key_type key_from_tom = tree->template get<key_type>(ptree_path_type{node_path + "/key", '/'});

                    auto eq_range = key_priority_map.equal_range(key_from_tom);

                    if (eq_range.first != eq_range.second) {
                        // Priority is higher - remove all previously inserted elements
                        if (current_priority > eq_range.first->second) {
                            key_priority_map.erase(eq_range.first, eq_range.second);
                        }

                        if (current_priority >= eq_range.first->second ) {
                            key_priority_map.emplace(key_from_tom, current_priority);
                        }
                    } else {
                        // No such a key - insert it
                        key_priority_map.emplace(key_from_tom, current_priority);
                    }
                }
            } catch ( ptree::ptree_bad_path& ) {} // TODO: use get_optional instead of throwing an exception
        };

        basic_operation</*Writer?*/false>(path, body, keys_with_priority);

        std::unordered_multiset<key_type> uset;

        // Cut unnecessary priority from the map
        for (auto& element : keys_with_priority) {
            uset.emplace(std::move(element.first));
        }

        return uset;
    }

    std::unordered_multimap<key_type, std::pair<mapped_type, priority_type>> internal_value_read( const path_type& path ) {
        std::unordered_multimap<key_type, std::pair<mapped_type, priority_type>> keys_with_priority;

        auto body = []( const path_type& node_path, ptree::ptree* tree,
                        priority_type current_priority,
                        std::unordered_multimap<key_type, std::pair<mapped_type, priority_type>>& key_priority_map ) {
            try {
                auto date_created = tree->template get_optional<date_type>(ptree_path_type{node_path + "/date_created", '/'});
                auto lifetime = tree->template get_optional<date_type>(ptree_path_type{node_path + "/lifetime", '/'});

                bool is_outdated = false;

                // If the lifetime for the key is presented
                if (date_created && lifetime) {
                    is_outdated = (std::chrono::system_clock::now() - std::chrono::seconds(lifetime.value())) >
                                  std::chrono::system_clock::time_point(std::chrono::seconds(date_created.value()));
                }

                if (!is_outdated) {
                    key_type key_from_tom = tree->template get<key_type>(ptree_path_type{node_path + "/key", '/'});
                    mapped_type mapped_from_tom = tree->template get<mapped_type>(ptree_path_type{node_path + "/mapped", '/'});

                    auto eq_range = key_priority_map.equal_range(key_from_tom);

                    if (eq_range.first != eq_range.second) {
                        // Priority is higher - remove all previously inserted elements
                        if (current_priority > eq_range.first->second.second) {
                            key_priority_map.erase(eq_range.first, eq_range.second);
                        }

                        if (current_priority >= eq_range.first->second.second) {
                            key_priority_map.emplace(std::piecewise_construct,
                                                    std::forward_as_tuple(key_from_tom),
                                                    std::forward_as_tuple(mapped_from_tom, current_priority));
                        }
                    } else {
                        key_priority_map.emplace(std::piecewise_construct,
                                                    std::forward_as_tuple(key_from_tom),
                                                    std::forward_as_tuple(mapped_from_tom, current_priority));
                    }
                }
            } catch( ptree::ptree_bad_path& ) {}
        };

        basic_operation</*Writer?*/false>(path, body, keys_with_priority);
        return keys_with_priority;
    }

    std::unordered_multiset<mapped_type> internal_mapped( const path_type& path ) {
        auto keys_with_priority = internal_value_read(path);

        std::unordered_multiset<mapped_type> uset;

        // Cut unnecessary priority and key from the map
        for (auto& element : keys_with_priority) {
            uset.emplace(std::move(element.second.first));
        }

        return uset;
    }

    std::unordered_multimap<key_type, mapped_type> internal_value( const path_type& path ) {
        auto keys_with_priority = internal_value_read(path);

        std::unordered_multimap<key_type, mapped_type> umap;

        // Cur unnecessary priority from the map
        for (auto& element : keys_with_priority) {
            umap.emplace(std::move(element.first),
                         std::move(element.second.first));
        }
        return umap;
    }

    template <bool AsNew, typename Predicate>
    std::size_t basic_modify_key( const path_type& path, const Predicate& pred ) {
        std::size_t modified_keys_counter = 0;

        auto body = [&modified_keys_counter, &pred]( const path_type& node_path, ptree::ptree* tree,
                                                     priority_type ) {
            try {
                bool modification_allowed = true;
                if constexpr (!AsNew) {
                    auto date_created = tree->template get_optional<date_type>(ptree_path_type{node_path + "/date_created", '/'});
                    auto lifetime = tree->template get_optional<date_type>(ptree_path_type{node_path + "/lifetime", '/'});

                    if (date_created && lifetime) {
                        modification_allowed = (std::chrono::system_clock::now() - std::chrono::seconds(lifetime.value())) <=
                                               std::chrono::system_clock::time_point(std::chrono::seconds(date_created.value()));
                    }
                }

                if (modification_allowed) {
                    auto path_to_key = ptree_path_type{node_path + "/key", '/'};
                    key_type key_from_tom = tree->template get<key_type>(path_to_key); // Throws in case of invalid path
                    tree->put(path_to_key, pred(key_from_tom));

                    if constexpr (AsNew) {
                        // If the key is considered as new - modify the time created as current time
                        tree->put(ptree_path_type{node_path + "/date_created", '/'},
                                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    }

                    ++modified_keys_counter;
                }
            } catch ( ptree::ptree_bad_path& ) {}
        };

        basic_operation</*Writer?*/true>(path, body);

        return modified_keys_counter;
    }

    template <typename Predicate>
    std::size_t internal_modify_key( const path_type& path, const Predicate& pred ) {
        return basic_modify_key</*as new = */false>(path, pred);
    }

    template <typename Predicate>
    std::size_t internal_modify_key_as_new( const path_type& path, const Predicate& pred ) {
        return basic_modify_key</*as new = */true>(path, pred);
    }

    template <bool AsNew, typename Predicate>
    std::size_t basic_modify_mapped( const path_type& path, const Predicate& pred ) {
        std::size_t modified_mapped_counter = 0;

        auto body = [&modified_mapped_counter, &pred]( const path_type& node_path, ptree::ptree* tree,
                                                       priority_type ) {
            try {
                bool modification_allowed = true;

                if constexpr (!AsNew) {
                    auto date_created = tree->template get_optional<date_type>(ptree_path_type{node_path + "/date_created", '/'});
                    auto lifetime = tree->template get_optional<date_type>(ptree_path_type{node_path + "/lifetime", '/'});

                    if (date_created && lifetime) {
                        modification_allowed = (std::chrono::system_clock::now() - std::chrono::seconds(lifetime.value())) <=
                                               std::chrono::system_clock::time_point(std::chrono::seconds(date_created.value()));
                    }
                }

                if (modification_allowed) {
                    auto path_to_mapped = ptree_path_type{node_path + "/mapped", '/'};
                    mapped_type mapped_from_tom = tree->template get<mapped_type>(path_to_mapped); // Throws in case of invalid path
                    tree->put(path_to_mapped, pred(mapped_from_tom));

                    if constexpr (AsNew) {
                        // If the key is considered as new - modify the time created as current time
                        tree->put(ptree_path_type{node_path + "/date_created", '/'},
                                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    }

                    ++modified_mapped_counter;
                }
            } catch( ptree::ptree_bad_path& ) {}
        };

        basic_operation</*Writer?*/true>(path, body);

        return modified_mapped_counter;
    }

    template <typename Predicate>
    std::size_t internal_modify_mapped( const path_type& path, const Predicate& pred ) {
        return basic_modify_mapped</*as new = */false>(path, pred);
    }

    template <typename Predicate>
    std::size_t internal_modify_mapped_as_new( const path_type& path, const Predicate& pred ) {
        return basic_modify_mapped</*as new = */true>(path, pred);
    }

    template <bool AsNew, typename Predicate>
    std::size_t basic_modify_value( const path_type& path, const Predicate& pred ) {
        std::size_t modified_value_counter = 0;

        auto body = [&modified_value_counter, &pred]( const path_type& node_path, ptree::ptree* tree,
                                                      priority_type ) {
            try {
                bool modification_allowed = true;
                if constexpr (!AsNew) {
                    auto date_created = tree->template get_optional<date_type>(ptree_path_type{node_path + "/date_created", '/'});
                    auto lifetime = tree->template get_optional<date_type>(ptree_path_type{node_path + "/lifetime", '/'});

                    if (date_created && lifetime) {
                        modification_allowed = (std::chrono::system_clock::now() - std::chrono::seconds(lifetime.value())) <=
                                               std::chrono::system_clock::time_point(std::chrono::seconds(date_created.value()));
                    }
                }

                if (modification_allowed) {
                    auto path_to_key = ptree_path_type{node_path + "/key", '/'};
                    // We can only check the path validity for key path
                    key_type key_from_tom = tree->template get<key_type>(path_to_key);
                    mapped_type mapped_from_tom = tree->template get<mapped_type>(ptree_path_type{node_path + "/mapped", '/'});
                    value_type modified_value = pred(value_type(key_from_tom, mapped_from_tom));
                    tree->put(path_to_key, modified_value.first);
                    tree->put(ptree_path_type{node_path + "/mapped", '/'}, modified_value.second);

                    if constexpr (AsNew) {
                        // If the key is considered as new - modify the time created as current time
                        tree->put(ptree_path_type{node_path + "/date_created", '/'},
                                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    }

                    ++modified_value_counter;
                }
            } catch( ptree::ptree_bad_path& ) {}
        };

        basic_operation</*Writer?*/true>(path, body);

        return modified_value_counter;
    }

    template <typename Predicate>
    std::size_t internal_modify_value( const path_type& path, const Predicate& pred ) {
        return basic_modify_value</*as new = */false>(path ,pred);
    }

    template <typename Predicate>
    std::size_t internal_modify_value_as_new( const path_type& path, const Predicate& pred ) {
        return basic_modify_value</*as new = */true>(path, pred);
    }

    bool basic_insert( const path_type& path, const value_type& value,
                       const std::chrono::seconds* lifetime_ptr )
    {
        bool inserted = false;

        auto body = [&]( const path_type& node_path, ptree::ptree* tree,
                         priority_type, const value_type& value ) {
            auto path_to_key = ptree_path_type{node_path + "/key", '/'};
            // Check if the path is already in a tree
            auto key = tree->template get_optional<key_type>(path_to_key);

            bool insertion_allowed = true;
            if (key) {
                // Key exists
                // But may be outdated
                auto date_created = tree->template get_optional<date_type>(ptree_path_type{node_path + "/date_created", '/'});
                auto lifetime = tree->template get_optional<date_type>(ptree_path_type{node_path + "/lifetime", '/'});

                if (date_created && lifetime) {
                    // If the key is outdated - insertion is allowed
                    insertion_allowed = (std::chrono::system_clock::now() - std::chrono::seconds(lifetime.value())) >
                                         std::chrono::system_clock::time_point(std::chrono::seconds(date_created.value()));
                } else {
                    // Key exists and not outdated - insertion is not allowed
                    insertion_allowed = false;
                }
            }

            if (insertion_allowed) {
                tree->put(path_to_key, value.first);
                tree->put(ptree_path_type{node_path + "/mapped", '/'}, value.second);
                if (lifetime_ptr != nullptr) {
                    tree->put(ptree_path_type{node_path + "/date_created", '/'},
                              std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    tree->put(ptree_path_type{node_path + "/lifetime", '/'},
                              lifetime_ptr->count());
                } else {
                    // Remove lifetime if any
                    tree->get_child(ptree_path_type{node_path, '/'}).erase("lifetime");
                }
                inserted = true;
            }
        };

        basic_operation</*Writer?*/true>(path, body, value);
        return inserted;
    }

    bool internal_insert( const path_type& path, const value_type& value ) {
        return basic_insert(path, value, nullptr);
    }

    bool internal_insert_with_lifetime( const path_type& path, const value_type& value,
                                        const std::chrono::seconds& lifetime )
    {
        return basic_insert(path, value, &lifetime);
    }

    bool internal_remove( const path_type& path ) {
        bool erased = false;
        auto body = [&]( const path_type& node_path, ptree::ptree* tree, priority_type ) {
            auto path_to_key = ptree_path_type{node_path + "/key", '/'};
            // Check if the path exists

            auto key = tree->template get_optional<key_type>(path_to_key);

            bool erasure_allowed = false;

            if (key) {
                // Key exists
                // But may be outdated
                auto date_created = tree->template get_optional<date_type>(ptree_path_type{node_path + "/date_created", '/'});
                auto lifetime = tree->template get_optional<date_type>(ptree_path_type{node_path + "/lifetime", '/'});

                if (date_created && lifetime) {
                    // If the key is outdated - erasure is not allowed
                    erasure_allowed = (std::chrono::system_clock::now() - std::chrono::seconds(lifetime.value())) <=
                                      std::chrono::system_clock::time_point(std::chrono::seconds(date_created.value()));
                } else {
                    // Key exists and not outdated - insertion is allowed
                    erasure_allowed = true;
                }
            }

            if (erasure_allowed) {
                // Cutting the last element after the delimiter
                path_type::const_iterator it;

                for (it = node_path.cend(); it != node_path.cbegin(); --it) {
                    if (*it == '/') break;
                }

                path_type path_to_prev_node(node_path.cbegin(), it);
                path_type curr_node_name(std::next(it), node_path.cend());

                tree->get_child(ptree_path_type{path_to_prev_node, '/'}).erase(curr_node_name);
                erased = true;
            }
        };

        basic_operation</*Write?*/true>(path, body);
        return erased;
    }

    void internal_destroy() {
        my_mount_table.for_each([&]( typename mount_hash_table::value_type& value ) {
            mount_node* node = value.second.load(std::memory_order_relaxed);

            while(node != nullptr) {
                mount_node* node_to_remove = node;
                node = node->next();
                delete_mount_node(node_to_remove);
            }
        });
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
