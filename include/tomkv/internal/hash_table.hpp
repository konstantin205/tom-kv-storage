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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAsGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __TOMKV_INCLUDE_INTERNAL_HASH_TABLE_HPP
#define __TOMKV_INCLUDE_INTERNAL_HASH_TABLE_HPP

#include "utils.hpp"
#include <shared_mutex>
#include <atomic>
#include <utility>
#include <mutex>

namespace tomkv {
namespace internal {

// TODO: add rehashing mechanism
template <typename Key, typename Mapped,
          typename Hasher, typename KeyEqual,
          typename Allocator>
class hash_table {
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<const key_type, mapped_type>;
    using allocator_type = Allocator;
    using allocator_traits_type = std::allocator_traits<allocator_type>;
    using hasher = Hasher;
    using key_equal = KeyEqual;
    using size_type = std::size_t;
public:
    class read_accessor;
    class write_accessor;

    hash_table( hasher& h,
                key_equal& key_eq,
                allocator_type& allocator )
        : my_allocator(allocator),
          my_hasher(h),
          my_equality(key_eq),
          my_bucket_count(/*init bucket count = */8),
          my_size(0),
          my_rehash_required_flag(false),
          my_segment_table(create_table()) {}

    // TODO: define copy/move constructors/assignments
    hash_table( const hash_table& );
    hash_table( hash_table&& );

    hash_table& operator=( const hash_table& );
    hash_table& operator=( hash_table&& );

    ~hash_table() {
        destroy_table();
    }

    template <typename... Args>
    bool emplace( read_accessor& acc, Args&&... args ) {
        return internal_emplace(acc, std::forward<Args>(args)...);
    }

    template <typename... Args>
    bool emplace( write_accessor& acc, Args&&... args ) {
        return internal_emplace(acc, std::forward<Args>(args)...);
    }

    template <typename... Args>
    bool emplace( Args&&... args ) {
        read_accessor racc;
        return internal_emplace(racc, std::forward<Args>(args)...);
    }

    bool find( read_accessor& acc, const key_type& key ) {
        return internal_find(acc, key);
    }

    bool find( write_accessor& acc, const key_type& key ) {
        return internal_find(acc, key);
    }

    bool erase( const key_type& key ) {
        return internal_erase(key);
    }

    void erase( write_accessor& acc ) {
        return internal_erase(acc);
    }

    size_type size() const {
        return my_size.load(std::memory_order_relaxed);
    }

    bool empty() const { return size() == 0; }

private:
    class node {
    public:
        template <typename... Args>
        node( Args&&... args ) : my_next(nullptr), my_value(std::forward<Args>(args)...) {}

        const key_type& key() const { return my_value.first; }
        mapped_type& mapped() { return my_value.second; }
        value_type& value() { return my_value; }

        void set_next( node* n ) { my_next = n; }
        node* next() const { return my_next; }
    private:
        node* my_next;
        value_type my_value;
    }; // class node

    class bucket {
    public:
        bucket() noexcept : my_list(nullptr) {}

        using read_lock_type = std::shared_lock<std::shared_mutex>;
        using write_lock_type = std::unique_lock<std::shared_mutex>;

        std::shared_mutex& mutex() { return my_mutex; }

        node* load_list() const {
            return my_list.load(std::memory_order_acquire);
        }

        void store_list( node* n ) {
            my_list.store(n, std::memory_order_release);
        }

        // Need to pass the expected head pointer because
        // actual head may be changed between search and try_insert
        bool try_insert( node* head, node* new_node ) {
            new_node->set_next(head);
            return my_list.compare_exchange_strong(head, new_node);
        }
    private:
        std::shared_mutex my_mutex;
        std::atomic<node*> my_list;
    }; // class bucket

    template <typename Derived>
    class accessor_base {
    public:
        accessor_base() : my_node(nullptr) {}

        void release() {
            Derived* der_this = static_cast<Derived*>(this);
            auto& der_lock = der_this->lock();
            if (der_lock) {
                der_lock.unlock();
                my_node = nullptr;
            }
        }

        void assign( node* n ) { my_node = n; }

        const key_type& key() const { return my_node->key(); }
    protected:
        node* get_node() const { return my_node; }
    private:
        node* my_node;
    }; // class accessor_base

public:
    class read_accessor : public accessor_base<read_accessor> {
    public:
        read_accessor() = default;
        read_accessor( read_accessor&& ) = default;

        ~read_accessor() { this->release(); }
        const mapped_type& mapped() const { return get_node()->mapped(); }
        // hazardous prefix means that the data race may accur while writing to
        // the non-const mapped or value object and some additional synchronization
        // is required on the user side to prevent it
        mapped_type& hazardous_mapped() { return get_node()->mapped(); }
        const value_type& value() const { return get_node()->value(); }
        value_type& hazardous_value() { return get_node()->value(); }

    private:
        typename bucket::read_lock_type& lock() { return my_lock; }

        using accessor_base<read_accessor>::assign;
        using accessor_base<read_accessor>::get_node;

        void assign( std::shared_mutex& m, node* n ) {
            my_lock = typename bucket::read_lock_type{m};
            assign(n);
        }

        typename bucket::read_lock_type my_lock;

        friend class accessor_base<read_accessor>;
        friend class hash_table;
    }; // class read_accessor

    class write_accessor : public accessor_base<write_accessor> {
    public:
        write_accessor() = default;
        write_accessor( write_accessor&& ) = default;

        ~write_accessor() { this->release(); }
        mapped_type& mapped() { return this->get_node()->mapped(); }
        value_type& value() const { return this->get_node()->value(); }

    private:
        typename bucket::write_lock_type& lock() { return my_lock; }

        using accessor_base<write_accessor>::assign;
        using accessor_base<write_accessor>::get_node;

        void assign( std::shared_mutex& m, node* n ) {
            my_lock = typename bucket::write_lock_type{m};
            this->assign(n);
        }

        typename bucket::write_lock_type my_lock;

        friend class accessor_base<write_accessor>;
        friend class hash_table;
    }; // class write_accessor

private:
    using segment_type = std::atomic<bucket*>;

    // Allocator types
    using bucket_allocator_type = typename allocator_traits_type::template rebind_alloc<bucket>;
    using table_allocator_type = typename allocator_traits_type::template rebind_alloc<segment_type>;
    using node_allocator_type = typename allocator_traits_type::template rebind_alloc<node>;

    // Allocator traits type
    using bucket_allocator_traits = std::allocator_traits<bucket_allocator_type>;
    using table_allocator_traits = std::allocator_traits<table_allocator_type>;
    using node_allocator_traits = std::allocator_traits<node_allocator_type>;

    static constexpr float max_load_factor = 1.0;

    // Returns an index of the segment in which the bucket with global_index is stored
    static constexpr size_type index_in_the_table( size_type global_index ) {
        return size_type(utils::log2(global_index | 1));
    }

    // Returns an index of the first bucket stored in the segment with specified index
    static constexpr size_type first_index_in_segment( size_type segment_index ) {
        return size_type(1) << segment_index & ~(size_type(1));
    }

    // Returns a number of buckets stored in the segment with specified index
    static constexpr size_type size_of_the_segment( size_type segment_index ) {
        return segment_index == 0 ? 2 : (size_type(1) << segment_index);
    }

    // Returns a number of segments in the segment table
    static constexpr size_type size_of_the_table() {
        return sizeof(size_type) * 8;
    }

    bucket* get_bucket( size_type bucket_index ) {
        size_type segment_index = index_in_the_table(bucket_index);
        create_segment_if_necessary(segment_index);
        size_type index_in_the_segment = bucket_index - first_index_in_segment(segment_index);

        return my_segment_table[segment_index].load(std::memory_order_relaxed) + index_in_the_segment;
    }

    template <typename... Args>
    node* create_node( Args&&... args ) {
        // TODO: store node allocator in the state, get rid of rebinding
        node_allocator_type node_allocator{my_allocator};
        node* new_node = node_allocator_traits::allocate(node_allocator, 1);
        // Deallocate the node if the pair constructor throws
        utils::raii_guard guard{[&]{
            node_allocator_traits::deallocate(node_allocator, new_node, 1);
        }};
        node_allocator_traits::construct(node_allocator, new_node, std::forward<Args>(args)...);
        guard.release();
        return new_node;
    }

    void destroy_node(node* n) {
        node_allocator_type node_allocator{my_allocator};
        node_allocator_traits::destroy(node_allocator, n);
        node_allocator_traits::deallocate(node_allocator, n, 1);
    }

    // Not thread-safe
    segment_type* create_table() const {
        table_allocator_type table_allocator{my_allocator};
        segment_type* table = table_allocator_traits::allocate(table_allocator, size_of_the_table());

        for (size_type i = 0; i < size_of_the_table(); ++i) {
            // std::atomic(T desired) is noexcept -> no extra care needed in this case
            table_allocator_traits::construct(table_allocator, table + i, nullptr);
        }
        return table;
    }

    // Not thread-safe
    void destroy_table() {
        table_allocator_type table_allocator(my_allocator);

        for (size_type i = 0; i < size_of_the_table(); ++i) {
            auto segment = my_segment_table[i].load(std::memory_order_relaxed);
            if (segment) {
                destroy_segment(segment, i);
            }
            // Destroy atomic pointer to the segment
            table_allocator_traits::destroy(table_allocator, my_segment_table + i);
        }
        table_allocator_traits::deallocate(table_allocator, my_segment_table, size_of_the_table());
    }

    void create_segment_if_necessary( size_type segment_index ) {
        if (my_segment_table[segment_index].load(std::memory_order_relaxed) == nullptr) {
            bucket_allocator_type bucket_allocator{my_allocator};
            bucket* buckets_in_segment = bucket_allocator_traits::allocate(bucket_allocator, size_of_the_segment(segment_index));

            for (size_type i = 0; i < size_of_the_segment(segment_index); ++i) {
                // Default constructor of the bucket is noexcept - no extra care needed
                bucket_allocator_traits::construct(bucket_allocator, buckets_in_segment + i);
            }

            bucket* expected = nullptr;
            if (!my_segment_table[segment_index].compare_exchange_strong(expected, buckets_in_segment,
                                                                         std::memory_order_relaxed,
                                                                         std::memory_order_relaxed))
            {
                // CAS failed => an other thread creates a segment
                for (size_type i = 0; i < size_of_the_segment(segment_index); ++i) {
                    bucket_allocator_traits::destroy(bucket_allocator, buckets_in_segment + i);
                }
                bucket_allocator_traits::deallocate(bucket_allocator, buckets_in_segment, size_of_the_segment(segment_index));
            }
        }
    }

    // Not thread-safe
    void destroy_segment( bucket* bucket_ptr, size_type segment_index ) {
        bucket_allocator_type bucket_allocator{my_allocator};

        for (size_type i = 0; i < size_of_the_segment(segment_index); ++i) {
            clear_bucket(bucket_ptr + i);

            bucket_allocator_traits::destroy(bucket_allocator, bucket_ptr + i);
        }
        bucket_allocator_traits::deallocate(bucket_allocator, bucket_ptr, size_of_the_segment(segment_index));
    }

    void clear_bucket( bucket* bucket_ptr ) {
        node* curr = bucket_ptr->load_list();

        while(curr) {
            node* n = curr;
            curr = curr->next();
            destroy_node(n);
        }
    }

    // Returns a pair where the first node is a node with equal key
    // the second node is the head of the bucket as a stop point in case we will need to search again
    std::pair<node*, node*> search_again( const key_type& key, bucket* b, node* stop_point ) {
        node* head = b->load_list();
        node* node = head;
        while(node != stop_point && !my_equality(key, node->key())) {
            node = node->next();
        }
        return node == stop_point ? std::pair{nullptr, head} : std::pair{node, head};
    }

    // Returns a pair where the first node is a node with equal key
    // the second node is the head of the bucket as a stop point in case we will need to search again
    std::pair<node*, node*> search( const key_type& k, bucket* b ) {
        // Search with nullptr as a stop point
        return search_again(k, b, nullptr);
    }

    // Should be executed if all buckets from 0 to current_bucket_count are locked
    void internal_rehash( size_type current_bucket_count ) {
        size_type new_bucket_count = current_bucket_count * 2;
        std::vector<node*> bucket_lists(current_bucket_count);

        // Clear all of the buckets
        for (size_type i = 0; i < current_bucket_count; ++i) {
            bucket* b = get_bucket(i);
            bucket_lists[i] = b->load_list();
            b->store_list(nullptr);
        }

        // Calculate new bucket indices for all elements in old bucket lists
        for (node* node_list : bucket_lists) {
            node* n = node_list;
            while(n != nullptr) {
                node* next = n->next();
                size_type new_bucket_index = my_hasher(n->key()) % new_bucket_count;
                bucket* new_bucket = get_bucket(new_bucket_index);
                bool inserted = new_bucket->try_insert(new_bucket->load_list(), n);
                __TOMKV_ASSERT(inserted);
                n = next;
            }
        }

        // All buckets are filled
        my_bucket_count.store(new_bucket_count, std::memory_order_release);
    }

    void rehash_if_necessary() {
        size_type current_bucket_count = my_bucket_count.load(std::memory_order_relaxed);

        if (my_rehash_required_flag.load(std::memory_order_acquire)) {
            std::vector<typename bucket::write_lock_type> locks(current_bucket_count);

            for (size_type i = 0; i < current_bucket_count; ++i) {
                locks[i] = typename bucket::write_lock_type{get_bucket(i)->mutex()};
            }

            // All buckets are locked for write
            // Test if rehashing is still required
            if (my_rehash_required_flag.load(std::memory_order_acquire) &&
                my_bucket_count.load(std::memory_order_relaxed) == current_bucket_count) {
                // Rehash is still required
                internal_rehash(current_bucket_count);
                // Rehashing done
                my_rehash_required_flag.store(false, std::memory_order_release);
            }
        }
    }

    void mark_rehash_required_if_necessary( size_type current_size, size_type current_bucket_count ) {
        if (float(current_size) / float(current_bucket_count) > max_load_factor) {
            my_rehash_required_flag.store(true, std::memory_order_release);
        }
    }

    template <typename Accessor, typename... Args>
    bool internal_emplace( Accessor& accessor, Args&&... args ) {
        accessor.release();

        rehash_if_necessary();

        node* new_node = create_node(std::forward<Args>(args)...);
        size_type hashcode = my_hasher(new_node->key());

        bucket* b = nullptr;
        size_type bc = my_bucket_count.load(std::memory_order_relaxed);

        while(true) {
            size_type prev_bc = bc;
            size_type bucket_index = hashcode % bc;
            b = get_bucket(bucket_index);
            accessor.assign(b->mutex(), new_node);
            // Lock is acquired
            bc = my_bucket_count.load(std::memory_order_relaxed);
            if (bc == prev_bc || hashcode % bc == bucket_index) {
                // Either no rehashing happened while acquiring the lock
                // or rehashing happened, but our new bucket is the same as an old one
                break;
            } else {
                // Rehashing happened while acquiring the lock
                // And current thread need to acquire an other bucket
                accessor.release(); // Release the lock
            }
        }

        // Lock scope

        // Seach an element with equal key in the bucket
        auto nodes = search(new_node->key(), b);

        if (nodes.first) {
            // An element with equal key already exists
            accessor.assign(nodes.second); // Re-assign the node in the accessor
            destroy_node(new_node);
            return false;
        }

        // No element with equal key - try to insert
        while(!nodes.first && !b->try_insert(nodes.second, new_node)) {
            // If the insertion fails - check once again
            // It is possible that an other thread already inserted an element with equal key
            nodes = search_again(new_node->key(), b, nodes.second);
        }

        if (nodes.first) {
            // An other thread inserted the node
            accessor.assign(nodes.first); // Re-assign the node
            destroy_node(new_node);
            return false;
        }
        // We have successfully inserted the node
        size_type sz = my_size.fetch_add(1, std::memory_order_relaxed);

        mark_rehash_required_if_necessary(sz + 1, bc);
        return true;
    }

    template <typename Accessor>
    bool internal_find( Accessor& accessor, const key_type& key ) {
        accessor.release();

        rehash_if_necessary();

        size_type hashcode = my_hasher(key);
        bucket* b = nullptr;

        size_type bc = my_bucket_count.load(std::memory_order_relaxed);

        while(true) {
            size_type prev_bc = bc;
            size_type bucket_index = hashcode % bc;
            b = get_bucket(bucket_index);
            accessor.assign(b->mutex(), nullptr);
            // Lock is acquired
            bc = my_bucket_count.load(std::memory_order_relaxed);
            if (bc == prev_bc || hashcode % bc == bucket_index) {
                // Either no rehashing happened while acquiring the lock
                // or rehashing happened, but our new bucket is the same as an old one
                break;
            } else {
                // Rehashing happened while acquirng the lock
                // And current thread need to acquire an other bucket
                accessor.release(); // Release the lock
            }
        }

        // Lock scope

        // Search the bucket to find an element with equal key
        node* n = search(key, b).first;
        if (n) {
            // Element found
            accessor.assign(n);
            return true;
        }

        // No such an element
        accessor.release();
        return false;
    }

    void internal_erase( write_accessor& accessor ) {
        size_type hashcode = my_hasher(accessor.key());

        size_type bc = my_bucket_count.load(std::memory_order_relaxed);
        bucket* b = get_bucket(hashcode % bc);

        // Lock scope
        node* prev = nullptr;
        node* curr = b->load_list(); // Head of the bucket

        // Find previous and current node in the list
        while(curr != accessor.get_node()) {
            prev = curr;
            curr = curr->next();
        }

        if (prev) {
            // Excluding node is not the head of the list
            prev->set_next(curr->next());
        } else {
            // Excluting node is the head of the list
            b->store_list(curr->next());
        }
        my_size.fetch_sub(1, std::memory_order_relaxed);
        destroy_node(curr);
        accessor.release();
    }

    bool internal_erase( const key_type& key ) {

        rehash_if_necessary();

        size_type hashcode = my_hasher(key);
        write_accessor accessor;

        size_type bc = my_bucket_count.load(std::memory_order_relaxed);
        bucket* b = nullptr;

        while(true) {
            size_type prev_bc = bc;
            size_type bucket_index = hashcode % bc;
            b = get_bucket(bucket_index);
            accessor.assign(b->mutex(), nullptr);
            // Lock is acquired
            bc = my_bucket_count.load(std::memory_order_relaxed);
            if (bc == prev_bc || hashcode % bc == bucket_index) {
                // Either no rehashing happened while acquiring the lock
                // or rehashing happened, but our new bucket is the same as an old one
                break;
            } else {
                // Rehashing happened while acquirng the lock
                // And current thread need to acquire an other bucket
                accessor.release(); // Release the lock
            }
        }

        // Lock scope

        node* prev = nullptr;
        node* curr = b->load_list(); // Head of the bucket

        // Find previous and current node in the list
        while(curr != nullptr && !my_equality(key, curr->key())) {
            prev = curr;
            curr = curr->next();
        }

        if (curr) {
            // Element found - exclude it from the list
            if (prev) {
                // Excluding node is not the head of the list
                prev->set_next(curr->next());
            } else {
                // Excluting node is the head of the list
                b->store_list(curr->next());
            }
            my_size.fetch_sub(1, std::memory_order_relaxed);
            destroy_node(curr);
            return true;
        }
        return false;
    }

    allocator_type&        my_allocator;
    hasher&                my_hasher;
    key_equal&             my_equality;
    std::atomic<size_type> my_bucket_count;
    std::atomic<size_type> my_size;
    std::atomic<bool> my_rehash_required_flag;
    segment_type*          my_segment_table;
}; // class hash_table

} // namespace internal
} // namespace tomkv

#endif // __TOMKV_INCLUDE_INTERNAL_HASH_TABLE_HPP
