# tomkv library

## Description

`tomkv` library provides several interfaces to manipulate with *toms*.

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

Each node of the tom contains key-value pair and all of its parent nodes. The node also can contains the date of node creation and the node lifetime. The purpose and detailed description of these parameters is described [here](./doc/storage.md).

`tomkv` library provides a class template `storage` that provides a view into one or several tom files using mounting mechanism.
`storage` permits multiple threads to concurrently mounting/unmounting nodes from toms and modifying key-value pairs inside of them.

Auxiliary functions from the `tomkv` library can be used to create empty toms and remove toms from the disc.

Also, the library provides a class template `unordered_map` which is semanticaly close to `std::unordered_map`, but allows
multiple threads to concurrently insert, find and erase elements inside of it.

## 5 minutes tutorial

[tomkv::storage](./doc/storage_tut.md)

[tomkv::unordered_map](./doc/unordered_map_tut.md)

## System requirements

`tomkv` library will work correctly with the following compilers (C++17 is required):

- GCC libstdc++ 6 and higher
- Clang libc++ 3.7 and higher
- MSVC 19.0 (Visual Studio 2015 U2) and higher

## Documentation

[tomkv::storage class template](./doc/storage.md)

[tomkv::unordered_map class template](./doc/unordered_map.md)

[Auxiliary functions](./doc/auxiliary.md)

## Performance benchmarks

`tomkv` library provides a small set of synthetic performance benchmarks (located in `bench` subdirectory) for its main components:

*Note*: to be able to build performance benchmarks using CMake, please set `TOMKV_BENCH` CMake option to `ON`.

[Benchmark for tomkv::storage](./doc/storage_bench.md)

[Benchmark for tomkv::unordered_map](./doc/unordered_map_bench.md)

## Functional tests

Functional tests for all `tomkv` library components are located in `test` subdirectory.

*Note*: to be able to build functional tests using CMake, please set `TOMKV_TEST` CMake option to `ON`.
