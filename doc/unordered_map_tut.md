# tomkv::unordered_map 5 minutes tutorial

The following example demonstrates basic principles of working with `tomkv::unordered_map` class template.

## Creating a map

There are multiple ways to create an empty `unordered_map` object. The simpliest was is to use the default constructor of the map:

```cpp
tomkv::unordered_map<int, int> umap;
```

If is also possible to provide some stateful or stateless hasher, key equality predicate and allocator.

## Inserting elements to the map

The following code is a simple way to insert the pair `1->1` into the map:

```cpp
bool inserted = umap.emplace(1, 1);
```

The key `1` is not presented in the map, so the insertion will return `true`.

If we try to insert the key `1` again:

```cpp
inserted = umap.emplace(1, 100);
```

the insertion will fail and return `false`.

## Accessors

Accessors (`read_accessor` and `write_accessor`) are helper classes providing read-only or write access into the item in the map.

`read_accessor` allows reading the element without data races. Any write operations on the element (or several elements) will wait until some read accessors are released or destroyed.

`write_accessor` allows reading and modifying the element without data races. Any operation on the element (or several elements) will wait untill some write accessors are relaeased or destroyed.

BKM is to release the accessor once the read/write access is no longer needed.

## Inserting the element with accessor

It is possible to insert the element and provide a reference to accessor as an additional argument:

```cpp
typename decltype(umap)::read_accessor racc;
inserted = umap.emplace(racc, 2, 2);
```

The key `2` is not presented in the map, so the insertion will be successful.

After insertion, read accessor `racc` will provide the access to the inserted element:

```cpp
std::cout << racc.key() << std::endl; // Will print 2
std::cout << racc.mapped() << std::endl; // Will print 2
auto val = racc.value(); // Returns a pair {2, 2}
```
## Lookup

Accessors are also useful if it is required to find some element in the map:

```cpp
bool found = umap.find(racc, 2);
```

This code founds an element with the key `2` in the map. In this example the lookup will be successful and `find` will return `true`.

Read accessor `racc` will provide read-only access to the found element.

Note that if the lookup is not successful, the accessor is not providing access to any element and reading values from it will result in undefined behavior.

## Erasure

`unordered_map` also provides an API for erasing elements with the specified key:

```cpp
bool erased = umap.erase(2);
```

The element with the key `2` will be removed from the map and the erasure will return `false`.

Also it it possible to pass `write_accessor` to the `erase` method to remove the corresponding element:

```cpp
typename decltype(umap)::write_accessor wacc;
if (umap.find(wacc, 1)) {
    umap.erase(wacc);
}
```
## Thread safety

Note that `tomkv::unordered_map` allows multiple threads to concurrently insert, erase and find elements.

It is really important to release accessors (via calling the `release()` method on accessor instance) to allow other threads to access the map.

For more detailed documentation see [this page](unordered_map.md).
