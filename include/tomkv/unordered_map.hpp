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

#ifndef __TOMKV_INCLUDE_HASH_TABLE_HPP
#define __TOMKV_INCLUDE_HASH_TABLE_HPP

#include "internal/hash_table.hpp"

namespace tomkv {
namespace internal {

template <typename Hash, typename KeyEqual, typename Allocator>
struct unordered_map_base {
    Hash my_key_hasher;
    KeyEqual my_key_equality;
    Allocator my_value_allocator;
}; // class unordered_map_base

template <typename Key, typename Mapped,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Mapped>>>
class unordered_map : public unordered_map_base<Hash, KeyEqual, Allocator>, public hash_table<Key, Mapped, Hash, KeyEqual, Allocator> {
    using unordered_base_type = unordered_map_base<Hash, KeyEqual, Allocator>;
    using hash_table_base_type = hash_table<Key, Mapped, Hash, KeyEqual, Allocator>;
public:
    unordered_map( const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() )
        : unordered_base_type{hash, equal, alloc},
          hash_table_base_type(this->my_key_hasher, this->my_key_equality, this->my_value_allocator) {}

    unordered_map( const Hash& hash, const Allocator& alloc )
        : unordered_map(hash, KeyEqual(), alloc) {}

    unordered_map( const Allocator& alloc )
        : unordered_map(Hash(), alloc) {}

    unordered_map( const unordered_map& other )
        : unordered_map(other, std::allocator_traits<Allocator>::select_on_container_copy_construction(other.my_value_allocator)) {}

    unordered_map( const unordered_map& other, const Allocator& alloc )
        : unordered_base_type{other.my_key_hasher, other.my_key_equality, alloc},
          hash_table_base_type(other.bucket_count(), this->my_key_hasher, this->my_key_equality, this->my_value_allocator)
    {
        hash_table_base_type::internal_copy(other);
    }

    unordered_map( unordered_map&& other )
        : unordered_base_type{std::move(other.my_key_hasher), std::move(other.my_key_equality),
                              std::move(other.my_value_allocator)},
          hash_table_base_type(other.bucket_count(), this->my_key_hasher, this->my_key_equality, this->my_value_allocator)
    {
        hash_table_base_type::internal_move(std::move(other));
    }

    unordered_map( unordered_map&& other, const Allocator& alloc )
        : unordered_base_type{std::move(other.my_key_hasher), std::move(other.my_key_equality), alloc},
          hash_table_base_type(this->my_key_hasher, this->my_key_equality, this->my_value_allocator)
    {
        hash_table_base_type::internal_move_with_allocator(std::move(other), alloc);
    }

    unordered_map& operator=( const unordered_map& other ) {
        if (this != &other) {
            this->my_key_hasher = other.my_key_hasher;
            this->my_key_equality = other.my_key_equality;

            if constexpr (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
                this->my_value_allocator = other.my_value_allocator;
            }
            hash_table_base_type::internal_copy_assign(other);
        }
        return *this;
    }

    unordered_map& operator=( unordered_map&& other ) {
        if (this != &other) {
            this->my_key_hasher = std::move(other.my_key_hasher);
            this->my_key_equality = std::move(other.my_key_equality);

            if constexpr (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                this->my_value_allocator = std::move(other.my_value_allocator);
            }
            hash_table_base_type::internal_move_assign(std::move(other));
        }
        return *this;
    }
}; // class unordered_map

} // namespace internal

using internal::unordered_map;

} // namespace tomkv

#endif // __TOMKV_INCLUDE_HASH_TABLE_HPP
