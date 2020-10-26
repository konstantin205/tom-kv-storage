# tomkv::unordered_map class template

`tomkv::unordered_map` class template is an associative container that stores key-value pairs with unique keys.

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
}; // class unordered_map

} // namespace tomkv
```

## Detailed description

### Template parameters

TODO: add description

### Member classes

#### read_accessor class

Class `tomkv::unordered_map::read_accessor` is a class that provides read-only access to the element inside the `unordered_map`.

`read_accessor` is called empty if it is not providing access to any element in `unordered_map`.

##### Synopsis

```cpp
namespace tomkv {

template <typename Key, typename Mapped, typename Hash, typename KeyEqual, typename Allocator>
class unordered_map<Key, Mapped, Hash, KeyEqual, Allocator>::read_accessor {
public:
    // Constructors
    read_accessor();
    read_accessor( read_accessor&& other );

    // Destructor
    ~read_accessor();

    // Observers
    const key_type& key() const;
    const mapped_type& mapped() const;
    const value_type& value() const;

    mapped_type& hazardous_mapped();
    value_type& hazardous_value();

    // Releasing
    void release();
};

} // namespace tomkv
```

##### Detailed description

###### Constructors

```cpp
read_accessor();
```

Constructs an empty accessor.

--------------------------------------------------------------

```cpp
read_accessor( read_accessor&& other );
```

Constructs an accessor by transfering ownership from `other` to the constructed accessor.

`other` is left in an empty state.

###### Destructor

Destroys the `read_accessor`.

An element, access for which is provided by the accessor stays unchanged.

###### Observers

```cpp
const key_type& key() const;
```

**Returns: ** a constant reference to the key of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
const mapped_type& mapped() const;
```
**Returns: ** a constant reference to the "mapped" of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
const value_type& value() const;
```

**Returns: ** a constant reference to the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
mapped_type& hazardous_mapped();
```

**Returns: ** a non-constant reference to the "mapped" of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

The `hazardous_` prefix means that modifying the returned non-constant reference can result in undefined behaviour because of data races (other threads can simultaneously read the same element). Some extra synchronization on the user-side is required to prevent data races in this point.

--------------------------------------------------------------

```cpp
value_type& hazardous_value();
```

**Returns: ** a non-constant reference to the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

The `hazardous_` prefix means that modifying the returned non-constant reference can result in undefined behaviour because of data races (other threads can simultaneously read the same element). Some extra synchronization on the user-side is required to prevent data races in this point.

###### Releasing

```cpp
void release();
```

Releases the access to the element. Accessor is left in an empty state.

### Constructors

```cpp
unordered_map( const hasher& hash = hasher(),
               const key_equal& key_eq = key_equal(),
               const allocator_type& alloc = allocator_type() );
```

Creates an empty `tomkv::unordered_map` object. Associates specified hasher, key equality predicate and allocator with the created object.

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

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
unordered_map( const unordered_map& other, const allocator_type& alloc );
```

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
unordered_map( unordered_map&& other );
```

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
unordered_map( unordered_map&& other, const allocator_type& alloc );
```

TODO: add description after implementation is done

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

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
unordered_map& operator=( unordered_map&& other );
```

TODO: add description after implementation is done


### Insertion

```cpp
template <typename... Args>
bool emplace( read_accessor& acc, Args&&... args );
```

Releases the accessor `acc` and attempts to insert a new value into the `unordered_map`.

Value is constructed in-place from `args`.

If the insertion succeeds, i.e. there was no element with equivalent key - assigns the accessor `acc` to provide read-only access to the inserted element.

**Returns: ** `true` if the insertion succeeds, `false` otherwise.

--------------------------------------------------------------

```cpp
template <typename... Args>
bool emplace( write_accessor& acc, Args&&... args );
```

Releases the accessor `acc` and attempts to insert a new value into the `unordered_map`.

Value is constructed in-place from `args`.

If the insertion succeeds, i.e. there was no element with equivalent key - assigns the accessor `acc` to provide write access to the inserted element.

**Returns: ** `true` if the insertion succeeds, `false` otherwise.

--------------------------------------------------------------

```cpp
template <typename... Args>
bool emplace( Args&&... args );
```

Attempts to insert a new value into the `unordered_map`.

Value is constructed in-place from `args`.

**Returns: ** `true` if the insertion succeeds, `false` otherwise.

### Lookup

```cpp
bool find( read_accessor& acc, const key_type& key );
```

Releases the accessor `acc` and finds an element with the key equivalent to `key` in the `unordered_map`.

If an element is found - assigns the accessor `acc` to provide read-only access to the corresponding element.

**Returns: ** `true` if the element is found, `false` otherwise.

--------------------------------------------------------------

```cpp
bool find( write_accessor& acc, const key_type& key );
```

Releases the accessor `acc` and finds an element with the key equivalent to `key` in the `unordered_map`.

If an elemnet is found - assigns the accessor `acc` to provide write access to the corresponding element.

**Returns: ** `true` if the element is found, `false` otherwise.

### Erasure

```cpp
bool erase( const key_type& key );
```
Attempts to erase an element with the key equivalent to `key` from the `unordered_map`.

**Returns: ** `true` if the element was erased, `false` otherwise.

--------------------------------------------------------------

```cpp
void erase( write_accessor& acc );
```

Erases the element accessed by `acc` from the `unordered_map`.

The behaviour is undefined if the accessor is empty, i.e. not providing access to any element in `*this`.
