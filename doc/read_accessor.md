
# tomkv::unordered_map::read_accessor class

## Description

Class `tomkv::unordered_map::read_accessor` is a class that provides read-only access to the element inside the `unordered_map`.

`read_accessor` is called empty if it is not providing access to any element in `unordered_map`.

## Synopsis

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
}; // class read_accessor

} // namespace tomkv
```

## Detailed description

### Constructors

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

### Destructor

```cpp
~read_accessor();
```

Destroys the `read_accessor`.

An element, access for which is provided by the accessor stays unchanged.

### Observers

```cpp
const key_type& key() const;
```

**Returns:** a constant reference to the key of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
const mapped_type& mapped() const;
```
**Returns:** a constant reference to the "mapped" of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
const value_type& value() const;
```

**Returns:** a constant reference to the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
mapped_type& hazardous_mapped();
```

**Returns:** a non-constant reference to the "mapped" of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

The `hazardous_` prefix means that modifying the returned non-constant reference can result in undefined behaviour because of data races (other threads can simultaneously read the same element). Some extra synchronization on the user-side is required to prevent data races in this point.

--------------------------------------------------------------

```cpp
value_type& hazardous_value();
```

**Returns:** a non-constant reference to the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

The `hazardous_` prefix means that modifying the returned non-constant reference can result in undefined behaviour because of data races (other threads can simultaneously read the same element). Some extra synchronization on the user-side is required to prevent data races in this point.

### Releasing

```cpp
void release();
```

Releases the access to the element. Accessor is left in an empty state.
