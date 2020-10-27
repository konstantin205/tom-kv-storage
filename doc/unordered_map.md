# tomkv::unordered_map class template

## Description

`tomkv::unordered_map` class template for an associative container that stores key-value pairs with unique keys, organized into buckets.

`tomkv::unordered_map` is semanticaly similar to `std::unordered_map`, but permits multiple threads to concurrently insert, find and erase elements.

## Header

```cpp
#include <tomkv/unordered_map.hpp>
```

## Class template synopsis

```cpp
namespace tomkv {

template <typename Key,
          typename Mapped,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Mapped>>>
class unordered_map {
public:
    // Member types
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<const key_type, mapped_type>;
    using allocator_type = Allocator;
    using hasher = Hasher;
    using key_equal = KeyEqual;
    using size_type = std::size_t;

    // Constructors
    unordered_map( size_type bc,
                   const hasher& hash = hasher(),
                   const key_equal& eq = key_equal(),
                   const allocator_type& alloc = allocator_type() )

    unordered_map( const hasher& hash = hasher(),
                   const key_equal& eq = key_equal(),
                   const allocator_type& alloc = allocator_type() );

    unordered_map( const hasher& hash, const allocator_type& alloc );

    unordered_map( const allocator_type& alloc );

    unordered_map( const unordered_map& other );
    unordered_map( const unordered_map& other, const allocator_type& alloc );

    unordered_map( unordered_map&& other );
    unordered_map( unordered_map&& other, const allocator_type& alloc );

    // Destructor
    ~unordered_map();

    // Assignment operators
    unordered_map& operator=( const unordered_map& other );
    unordered_map& operator=( unordered_map&& other );

    // Member classes
    class read_accessor;
    class write_accessor;

    // Observers
    allocator_type get_allocator() const;
    size_type size() const;
    bool empty() const;

    // Insertion
    template <typename... Args>
    bool emplace( read_accessor& acc, Args&&... args );

    template <typename... Args>
    bool emplace( write_accessor& acc, Args&&... args );

    template <typename... Args>
    bool emplace( Args&&... args );

    // Lookup
    bool find( read_accessor& acc, const key_type& key );

    bool find( write_accessor& acc, const key_type& key );

    // Erasure
    bool erase( const key_type& key );

    void erase( write_accessor& acc );

    // Auxiliary functions
    // Not thread-safe
    template <typename Predicate>
    void for_each( const Predicate& pred );
}; // class unordered_map

} // namespace tomkv
```

## Detailed description

### Member classes

[read_accessor class](./read_accessor.md)

[write_accessor class](./write_accessor.md)

### Constructors

```cpp
unordered_map( size_type bc,
               const hasher& hash = hasher(),
               const key_equal& key_eq = key_equal(),
               const allocator_type& alloc = allocator_type() );
```

Creates an empty `tomkv::unordered_map` object with `bc` buckets. Associates specified hasher, key equality predicate and allocator with the created object.

```cpp
unordered_map( const hasher& hash = hasher(),
               const key_equal& key_eq = key_equal(),
               const allocator_type& alloc = allocator_type() );
```

Creates an empty `tomkv::unordered_map` object. Associates specified hasher, key equality predicate and allocator with the created object.

The initial number of buckets is unspecified.

--------------------------------------------------------------

```cpp
unordered_map( const hasher& hash, const allocator_type& alloc );
```

Equivalent to `unordered_map(hash, key_equal(), alloc)`.

--------------------------------------------------------------

```cpp
unordered_map( const allocator_type& alloc );
```

Equivalent to `unordered_map(hasher(), key_equal(), alloc)`.

--------------------------------------------------------------

```cpp
unordered_map( const unordered_map& other );
```

Creates a copy of `other`. Copy-constructs the stored hasher and key equality predicate.

Associates the allocator obtained by `std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.get_allocator())` with the created object.

--------------------------------------------------------------

```cpp
unordered_map( const unordered_map& other, const allocator_type& alloc );
```

Creates a copy of `other`. Copy-constructs the stored hasher and key equality predicate.

Associates the allocator `alloc` with the created object.

--------------------------------------------------------------

```cpp
unordered_map( unordered_map&& other );
```

Creates an object by transfering all elements from `other` using move semantics.

Move-constructs the stored hasher, key equality predicate and allocator.

`other` is left in a valid, but unspecified state.

--------------------------------------------------------------

```cpp
unordered_map( unordered_map&& other, const allocator_type& alloc );
```

Creates an object by transfering all elements from `other` using move semantics.

Move-constructs the stored hasher and key equality predicate.

Associates the allocator `alloc` with the created object.

`other` is left in a valid, but unspecified state.

--------------------------------------------------------------

### Destructor

```cpp
~unordered_map();
```

Destroys the `tomkv::unordered_map` object and deallocates all used memory.

The behavior is undefined in case of any concurrent operations with the object which is destroying.

### Assignment operators

```cpp
unordered_map& operator=( const unordered_map& other );
```

Replaces all elements in `*this` by the copies of the elements in `other`.

Copy-assigns the stored hasher and key equality predicate.

Copy-assigns the stored allocator object if `std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value` is `true`.

**Returns:** a reference to `*this`.

--------------------------------------------------------------

```cpp
unordered_map& operator=( unordered_map&& other );
```

Replaces all elements in `*this` by the elements in `other` using move semantics.

Move-assigns the stored hasher and key equality predicate.

Move-assigns the stored allocator object if `std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value` is `true`.

`other` is left in a valid, but unspecified state.

**Returns:** a reference to `*this`.

### Observers

```cpp
allocator_type get_allocator() const;
```

**Returns:** a copy of the allocator, associated with the object.

--------------------------------------------------------------

```cpp
size_type size() const;
```

**Returns:** the number of elements in the container.

--------------------------------------------------------------

```cpp
bool empty() const;
```

**Returns:** `true` if the container is empty, `false` otherwise.

### Insertion

```cpp
template <typename... Args>
bool emplace( read_accessor& acc, Args&&... args );
```

Releases the accessor `acc` and attempts to insert a new value into the `unordered_map`.

Value is constructed in-place from `args`.

If the insertion succeeds, i.e. there was no element with equivalent key - assigns the accessor `acc` to provide read-only access to the inserted element.

**Returns:** `true` if the insertion succeeds, `false` otherwise.

--------------------------------------------------------------

```cpp
template <typename... Args>
bool emplace( write_accessor& acc, Args&&... args );
```

Releases the accessor `acc` and attempts to insert a new value into the `unordered_map`.

Value is constructed in-place from `args`.

If the insertion succeeds, i.e. there was no element with equivalent key - assigns the accessor `acc` to provide write access to the inserted element.

**Returns:** `true` if the insertion succeeds, `false` otherwise.

--------------------------------------------------------------

```cpp
template <typename... Args>
bool emplace( Args&&... args );
```

Attempts to insert a new value into the `unordered_map`.

Value is constructed in-place from `args`.

**Returns:** `true` if the insertion succeeds, `false` otherwise.

### Lookup

```cpp
bool find( read_accessor& acc, const key_type& key );
```

Releases the accessor `acc` and finds an element with the key equivalent to `key` in the `unordered_map`.

If an element is found - assigns the accessor `acc` to provide read-only access to the corresponding element.

**Returns:** `true` if the element is found, `false` otherwise.

--------------------------------------------------------------

```cpp
bool find( write_accessor& acc, const key_type& key );
```

Releases the accessor `acc` and finds an element with the key equivalent to `key` in the `unordered_map`.

If an elemnet is found - assigns the accessor `acc` to provide write access to the corresponding element.

**Returns:** `true` if the element is found, `false` otherwise.

### Erasure

```cpp
bool erase( const key_type& key );
```
Attempts to erase an element with the key equivalent to `key` from the `unordered_map`.

**Returns:** `true` if the element was erased, `false` otherwise.

--------------------------------------------------------------

```cpp
void erase( write_accessor& acc );
```

Erases the element accessed by `acc` from the `unordered_map`.

The behaviour is undefined if the accessor is empty, i.e. not providing access to any element in `*this`.

### Auxiliary functions

```cpp
template <typename Predicate>
void for_each( const Predicate& pred );
```

Applies the `pred` function object to all key-value pairs in the `unordered_map`.

The behaviour is undefined in case of any concurrent operations with the object.
