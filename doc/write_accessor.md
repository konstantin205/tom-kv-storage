# tomkv::unordered_map::write_accessor class

## Description

Class `tomkv::unordered_map::write_accessor` is a class that provides write access to the element inside the `unordered_map`.

`write_accessor` is called empty if it is not providing access to any element in `unordered_map`.

## Synopsis

```cpp
namespace tomkv {

template <typename Key, typename Mapped, typename Hash, typename KeyEqual, typename Allocator>
class unordered_map<Key, Mapped, Hash, KeyEqual, Allocator>::write_accessor {
public:
    // Constructors
    write_accessor();
    write_accessor( write_accessor&& other );

    // Destructor
    ~write_accessor();

    // Observers
    const key_type& key() const;
    mapped_type& mapped();
    value_type& value();

    // Releasing
    void release();
}; // class write_accessor

} // namespace tomkv
```

## Detailed description

### Constructors

```cpp
write_accessor();
```

Constructs an empty accessor.

--------------------------------------------------------------

```cpp
write_accessor( write_accessor&& other );
```

Constructs an accessor by transfering ownership from `other` to the constructed accessor.

`other` is left in an empty state.

### Destructor

```cpp
~write_accessor();
```

Destroys the `write_accessor`.

An element, access for which is provided by the accessor stays unchanged.

### Observers

```cpp
const key_type& key() const;
```

**Returns:** a constant reference to the key of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
mapped_type& mapped();
```

**Returns:** a non-constant reference to the "mapped" of the element, the access for which is provided by the accessor.

The behaviour is undefined if the accessor is empty.

--------------------------------------------------------------

```cpp
value_type& value();
```

**Returns:** a non-constant reference to the element, the access for which is provided by the accessor.

Teh behaviour is undefined if the accessor is empty.

### Releasing

```cpp
void release();
```

Releases the access to the element. Accessor is left in an empty state.
