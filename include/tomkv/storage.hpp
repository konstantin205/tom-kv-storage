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

#include "hash_table.hpp"
#include <string>
#include <optional>
#include <functional>
#include <utility>
#include <atomic>
#include <memory>

namespace tomkv {
namespace internal {

template <typename Key, typename Mapped>
class storage {
public:
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<key_type, mapped_type>;

    using mount_id = std::string;
    using tom_id = std::string;
    using path_type = std::string;
private:
    static_assert(std::is_same_v<mount_id, tom_id>);
    using id_hasher = std::hash<mount_id>;
    using id_equality = std::equal_to<mount_id>;

    using allocator_type = std::allocator<value_type>; // TODO: change allocator template
public:
    storage();

    storage( const storage& );
    storage( storage&& );

    ~storage();

    storage& operator=( const storage& );
    storage& operator=( storage&& );

    void mount( const mount_id&, const tom_id&, const path_type& );

    bool unmount( const mount_id& );

    std::optional<key_type> key( const path_type& );
    std::optional<mapped_type> mapped( const path_type& );
    std::optional<value_type> value( const path_type& );

    bool set_key( const path_type&, const key_type& );
    bool set_mapped( const path_type&, const mapped_type& );
    bool set_value( const path_type&, const value_type& );

    bool remove( const path_type& );

private:
    class mount_info;
    class tom_info;

    allocator_type                      my_allocator;
    id_hasher                           my_hasher;
    id_equality                         my_id_equality;
    hash_table<mount_id, mount_info>    my_mount_table;
    hash_table<tom_id, tom_info>        my_tom_table;
}; // class storage

} // namespace internal
} // namespace tomkv

#endif // __TOMKV_INCLUDE_STORAGE_HPP
