# tomkv::storage class template

## Description

`tomkv::storage` class template for a virtual node-based storage that mounts paths from *toms* its nodes.

*Tom* is an XML file contains key-value pairs as a hierarchial structure. The tom has the following format

```xml
<tom>
<root>
<node-id-1>
    <key>KEY</key>
    <mapped>MAPPED</mapped>
    <!--Optional--><date_created>DATE_CREATED</date_created>
    <!--Optional--><lifetime>LIFETIME</lifetime>
    <node-id-2>
        <!--Other nodes and values-->
    </node-id-2>
</node-id-1>
</root>
</tom>
```

Each node of the tom contains key-value pair and all of its parent nodes. The node also can contains the date of node creation and the node lifetime. The purpose and detailed description of these parameters will be described later.

`storage` class allows to mount some path from the tom to the storage, e.g. the path `a/b` can be mounted to the node with mount identificator `mnt`. It allows to access the data from the tom using the corresponding mount identificator.

`storage` also allows to mount multiple paths from one or several toms to the same mount identificator (with or without priority).

The class permits multiple threads to concurrently:
- Mount and unmount paths to some mount identificator
- Read and modify key-value pairs inside of the tom using mount_identificator
- Insert new nodes with or without lifetime
- Remove nodes from the tom

## Header

```cpp
#include <tomkv/storage.hpp>
```

## Example

```cpp
// Initially, tom1.xml is the file with the following content
// <tom><root>
// <a>
//      <key>1</key>
//      <mapped>100</mapped>
//      <b>
//          <key>2</key>
//          <mapped>200</mapped>
//          <c>
//              <key>3</key>
//              <mapped>300</mapped>
//          </c>
//      </b>
// </a>
// <b>
//      <key>4</key>
//      <mapped>400</mapped>
//      <c>
//          <key>5</key>
//          <mapped>500</mapped>
//      </c>
// </b>
// </root></tom>

#include <tomkv/storage.hpp>
#include <iostream>

int main() {
    tomkv::storage<int, int> st;

    // Mount nodes "a/b" and "b" to the mount identificator "mnt"

    st.mount("mnt", "tom1.xml", "a/b");
    st.mount("mnt", "tom1.xml", "b");

    // Read keys
    auto keys = st.key("mnt/c");
    std::cout << "Keys from mounted nodes: " << std::endl;
    for (auto key : keys) {
        // Should write keys 3 and 5
        std::cout << "\t" << key << std::endl;
    }

    // Modify values
    auto c = st.set_value("mnt/c", std::pair{400, 4000});
    std::cout << "Modified " << c << " values" << std::endl;

    // Read mapped from invalid path
    keys = st.key("mounted_path/c"); // Throws tomkv::unmounted_path
}
```

## Class template synopsis

```cpp
namespace tomkv {

template <typename Key,
          typename Mapped,
          typename Allocator = std::allocator<std::pair<Key, Mapped>>>
class storage {
public:
    // Member types
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<key_type, mapped_type>;

    using mount_id = std::string;
    using tom_id = std::string;
    using path_type = std::string;
    using priority_type = std::size_t;

    // Constructors
    storage( const allocator_type& alloc = allocator_type() );

    storage( const storage& other );
    storage( const storage& other, const allocator_type& alloc );

    storage( storage&& other );
    storage( storage&& other, const allocator_type& alloc );

    // Destructor
    ~storage();

    // Assignment operators
    storage& operator=( const storage& other );
    storage& operator=( storage&& other );

    // Mounting
    void mount( const mount_id& m_id,
                const tom_id& t_id,
                const path_type& path,
                priority_type priority = priority_type(0) );

    bool unmount( const mount_id& m_id );

    std::list<std::pair<tom_id, path_type>> get_mounts( const mount_id& m_id );

    // Observers
    std::unordered_multiset<key_type> key( const path_type& path );

    std::unordered_multiset<mapped_type> mapped( const path_type& path );

    std::unordered_multimap<key_type, mapped_type> value( const path_type& path );

    // Modifiers
    std::size_t set_key( const path_type& path, const key_type& key );
    std::size_t set_mapped( const path_type& path, const mapped_type& mapped );
    std::size_t set_value( const path_type& path, const value_type& value );

    template <typename Predicate>
    std::size_t modify_key( const path_type& path, const Predicate& pred );

    template <typename Predicate>
    std::size_t modify_mapped( const path_type& path, const Predicate& pred );

    template <typename Predicate>
    std::size_t modify_value( const path_type& path, const Predicate& pred );

    std::size_t set_key_as_new( const path_type& path, const key_type& key );
    std::size_t set_mapped_as_new( const path_type& path, const mapped_type& mapped );
    std::size_t set_value_as_new( const path_type& path, const value_type& value );

    template <typename Predicate>
    std::size_t modify_key_as_new( const path_type& path, const Predicate& pred );

    template <typename Predicate>
    std::size_t modify_mapped_as_new( const path_type& path, const Predicate& pred );

    template <typename Predicate>
    std::size_t modify_value_as_new( const path_type& path, const Predicate& pred );

    // Insertions
    bool insert( const path_type& path, const value_type& value );

    bool insert( const path_type& path, const value_type& value,
                 const std::chrono::seconds& lifetime );

    // Removal
    bool remove( const path_type& path );
}; // class storage

} // namespace tomkv
```

## Detailed description

### Outdated key-value pairs

Class `storage` supports automatic virtual removal of the key-value pairs inside of the toms.

Each node in the tom have optional parameters `<date_created>` and `<lifetime>`.

`<date_created>` contains a number of seconds obtained as `std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()`. It represents the number of seconds since `std::chrono::system_clock` epoch.

`<lifetime>` contains a number of seconds after which the key-value pair is considered outdated. Outdated key-value pairs are still presented in the tom, but `storage` observers will not see them.

### Constructors

```cpp
storage( const allocator_type& alloc );
```

Creates an empty `tomkv::storage` object. Associates specified allocator with the created object.

--------------------------------------------------------------

```cpp
storage( const storage& other );
```

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
storage( const storage& other, const allocator_type& alloc );
```

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
storage( storage&& other );
```

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
storage( storage&& other, const allocator_type& alloc );
```

TODO: add description after implementation is done

--------------------------------------------------------------

### Destructor

```cpp
~storage();
```

Destroys the `tomkv::storage` object and deallocates all used memory.

The behavior is undefined in case of any concurrent operations with the object which is destroying.

### Assignment operators

```cpp
storage& operator=( const storage& other );
```

TODO: add description after implementation is done

--------------------------------------------------------------

```cpp
storage& operator=( storage&& other );
```

TODO: add description after implementation is done

--------------------------------------------------------------

### Mounting

```cpp
void mount( const mount_id& m_id,
            const tom_id& t_id,
            const path_type& path,
            priority_type priority = priority_type(0) );
```

Mounts the node with the path `path` from the tom `t_id` into the storage with the priority `priority`. New node can be obtained using the mount identificator `m_id`.

Allows mounting multiple paths into the same mount_id.

--------------------------------------------------------------

```cpp
bool unmount( const mount_id& m_id );
```

Unmounts all paths with the mount identificator `m_id` from the storage. Mount identificator cannot be used to observe the storage any more.

**Returns:** `true` if at least one mount was removed, `false` otherwise.

--------------------------------------------------------------

```cpp
std::list<std::pair<tom_id, path_type>> get_mounts( const mount_id& m_id );
```

**Returns:** a list of all paths mounted using the mount identificator `m_id`.

Each element in the returned list is a pair in which the first element is the identifier of the tom and the second element is the real path inside of the tom.

### Observers

```cpp
std::unordered_multiset<key_type> key( const path_type& path );
```

Reads all not outdated key components from the path `path`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** a `std::unordered_multiset<key_type>` contains all not outdated keys with the path `path`.

It can contain multiple elements because the storage allows mounting multiple paths to the same mount identificator.

If there are multiple paths mounted with priority to the same identificator, and keys on several mounted real paths are equal, only the keys with the highest priority will be returned.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
std::unordered_multiset<mapped_type> mapped( const path_type& path );
```
Reads all not outdated mapped components from the path `path`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** `tomkv::unordered_multiset<mapped_type>` contains all not outdated mapped components with the path `path`.

It can contain multiple elements because the storage allows mounting multiple paths to the same mount identificator.

If there are multiple paths mounted with priority to the same identificator, and keys on several mounted real paths are equal, only the mapped components with the highest priority will be returned.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
std::unordered_multimap<key_type, mapped_type> value( const path_type& path );
```

Reads all not outdated key-value pairs from the path `path`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** a `std::unordered_multimap<key_type, mapped_type>` contains all not outdated key-value pairs with the path `path`.

It can contain multiple elements because the storage allows mounting multiple paths to the same mount identificator.

If there are multiple paths mounted with priority to the same identificator, and keys on several mounted real paths are equal, only the elements with the highest priority will be returned.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

### Modifiers

### Setting

```cpp
std::size_t set_key( const path_type& path, const key_type& key );
```

Sets the key for all not outdated nodes with the path `path` to `key`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
std::size_t set_mapped( const path_type& path, const mapped_type& mapped );
```

Sets the mapped component for all not outdated nodes with the path `path` to `mapped`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
std::size_t set_value( const path_type& path, const value_type& value );
```
Sets the key-value pair for all not outdated nodes with the path `path` to `value`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

### Setting as new

```cpp
std::size_t set_key_as_new( const path_type& path, const key_type& key );
```

Sets the key for all nodes(including outdated) with the path `path` to `key`.

In case of modifying the outdated key-value pair, the mapped component and the lifetime of the new key-value pair will be equal to the corresponding component from the outdated key-value pair.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
std::size_t set_mapped_as_new( const path_type& path, const mapped_type& mapped );
```

Sets the mapped component for all nodes(including outdated) with the path `path` to `mapped`.

In case of modifying the outdated key-value pair, the key component and the lifetime of the new key-value pair will be equal to the corresponding component from the outdated key-value pair.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
std::size_t set_value_as_new( const path_type& path, const value_type& value );
```

Sets the key-value pair for all nodes(including outdated) with the path `path` to `value`.

In case of modifying the outdated key-value pair, the lifetime of the new key-value pair will be equal to the lifetime of the outdated key-value pair.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

#### Modifying

```cpp
template <typename Predicate>
std::size_t modify_key( const path_type& path, const Predicate& pred );
```

Modifies key components for all not outdated nodes with the path `path` using the function object `pred`.

The new key will be equal to `pred(old_key)`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
template <typename Predicate>
std::size_t modify_mapped( const path_type& path, const Predicate& pred );
```

Modifies mapped components for all not outdated nodes with the path `path` using the function object `pred`.

The new mapped component will be equal to `pred(old_mapped)`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
template <typename Predicate>
std::size_t modify_value( const path_type& path, const Predicate& pred );
```

Modifies the key-value pair for all not outdated nodes with the path `path` using the function object `pred`.

The new key-value pair will be equal to `pred(old_kv_pair)`.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

#### Modifying as new

```cpp
template <typename Predicate>
std::size_t modify_key_as_new( const path_type& path, const Predicate& pred );
```

Modifies the key component for all nodes(including outdated) with the path `path` using the function object `pred`.

The new key will be equal to `pred(old_key)`.

In case of modifying the outdated key-value pair, the mapped component and the lifetime of the new key-value pair will be equal to the corresponding component from the outdated key-value pair.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
template <typename Predicate>
std::size_t modify_mapped_as_new( const path_type& path, const Predicate& pred );
```

Modifies the mapped component for all nodes(including outdated) with the path `path` using the function object `pred`.

The new mapped component will be equal to `pred(old_mapped)`.

In case of modifying the outdated key-value pair, the key component and the lifetime of the new key-value pair will be equal to the corresponding component from the outdated key-value pair.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
template <typename Predicate>
std::size_t modify_value_as_new( const path_type& path, const Predicate& pred );
```

Modifies the key-value pair for all nodes(including outdated) with the path `path` using the function object `pred`.

In case of modifying the outdated key-value pair, the lifetime of the new key-value pair will be equal to the lifetime of the outdated key-value pair.

Valid mount identificator should be a part of the `path`, e.g. if `path` is `mnt/some/path` on the of the `mnt`, `mnt/some` or `mnt/some/path` should be valid mount identificator (mounted using `mount` member function).

**Returns:** the number of elements modified.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

### Insertions

```cpp
bool insert( const path_type& path, const value_type& value );
```

Inserts the new node with the key-value pair `value` on the path `path` if it is not already presented.

**Returns:** `true` if the node was inserted, `false` otherwise.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

--------------------------------------------------------------

```cpp
bool insert( const path_type& path, const value_type& value,
             const std::chrono::seconds& lifetime );
```

Inserts the new node with the key-value pair `value` and the lifetime `lifetime` on the path `path` if it is not already presented.

After `lifetime` seconds, the key-value pair will be considered outdated.

**Returns:** `true` if the node was inserted, `false` otherwise.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.

### Removal

```cpp
bool remove( const path_type& path );
```

Removes the node with the path `path` if it is not outdated.

**Returns:** `true` if the node was removed, `false` otherwise.

**Throws:** `tomkv::unmounted_path` if there are no valid mount identificator as part of `path`.
