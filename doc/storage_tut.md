# tomkv::storage 5 minutes tutorial

The following example demonstrates basic principles of working with `tomkv::storage` class template.

## Creating a tom

Tom is an XML document which contains key-value pairs and some auxiliary information. It should have the following structure:

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

The initial content of the tom can be created manually or using some external libraries for writing XML (e.g. `boost::property_tree`).

It is also possible to create an empty tom using the auxiliary function `tomkv::create_empty_tom` and then add some elements manually.

In this page, consider the `tom1` as the file `tom1.xml` with the following content:

```xml
<tom><root>
<a>
    <key>1</key>
    <mapped>1</mapped>
    <b>
        <key>2</key>
        <mapped>2</mapped>
        <c>
            <key>3</key>
            <mapped>3</mapped>
        </c>
        <cc>
            <key>33</key>
            <mapped>33</mapped>
        </cc>
    </b>
</a>
<d>
    <key>4</key>
    <mapped>4</mapped>
    <c>
        <key>5</key>
        <mapped>5</mapped>
    <c>
</d>
</tom></root>
```

## Creating the storage

The easiest way to create a storage is to use the default construcor. It is also possible to pass an allocator object to the constructor.

```cpp
tomkv::storage<int, int> st;
```

This storage will consider the elements in `<key>` and `<mapped>` values in `tom1` as integers.

## Mounting the path from tom into storage

The following code mounts the path `tom/root/a/b` from tom `tom1` to the mount identificator `mnt`:

```cpp
st.mount("mnt", "tom1.xml", "a/b");
```

It is also possible to mount an other path `tom/root/d` from the same tom to the same mount identificator:

```cpp
st.mount("mnt", "tom1.xml", "d");
```

Starting from this point, two paths from `tom1` are mounted to the same mount identificator `mnt`.

## Key/mapped/value modification

It is possible to modify some key(mapped or value) using the corresponding path inside the tom.

The path should contain valid (mounted using `mount` member function) mount identificator.

The following code sets the mapped component to `42` using the path `mnt/c/cc`.

```cpp
std::size_t count = st.set_mapped("mnt/c/cc", 42);
```

The mount identificator `mnt` mounts two paths in the tom: `tom/root/a/b` and `tom/root/d`, but the path
`mnt/c/cc` will be valid only for the first mounted path (`tom/root/a/b/c/cc`).

Because of this, the code below will modify only one mapped component and return `1`.

Starting from this point, the value in the node `tom/root/a/b/c/cc/mapped` is equal to `42`.

If we try to modify the component using the path which do not contains the valid mount identificator, the `tomkv::unmounted_path`
exception will be thrown:

```cpp
st.set_mapped("mount/c/cc", 42); // Throws
```

## Inserting new nodes

It is also possible to insert (as well as remove) the node using the path, which also should contain valid mount identificator:

```cpp
bool inserted = st.insert("mnt/c/cc/ccc", std::pair{333, 333});
```

It adds the new node on the path `tom/root/a/b/c/cc` with the node identificator `ccc`.`<key>` and `<mapped>` components will be both equal to `333`.

If the specified path is already presented in the tom: the function `insert` will return `false`.

## Thread safety

Note that `tomkv::storage` class permits multiple threads to concurrently modify, insert and remove values to one or several toms as well as mount (or unmount) some mount identificators.

For more detailed documentation see [this page](storage.md).
