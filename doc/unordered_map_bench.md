# bench_unordered_map performance benchmark

`bench_unordered_map` is a performance benchmark for unordered maps. It allows setting the percentage of insert, lookup and erase operations as well as the number of threads and the number of operatins per each thread.

Benchmarks supports two possible unordered maps: `tomkv::unordered_map` and `std::unordered_map` with `std::mutex`.

## Command line options

`bench_unordered_map` supports the following command line options:

- `--help` - prints help message with possible command line options
- `--use-stl` - uses `std::unordered_map` with `std::mutex` instead of `tomkv::unordered_map`
- `--insert <value>` (mandatory) - the percentage of threads that inserts elements into the map
- `--find <value>` (mandatory) - the percentage of threads that finds elements in the map
- `--erase <value>` (mandatory) - the percentage of threads that erases elements from the map
- `--num-threads <value>` (optional) - the number of threads to use while benchmarking. The default value is the hardware concurrency of the current system.
- `--num-elements <value> ` (optional) - the number of elements to insert/find/erase by each thread. The default value is 1000.
- `--verbose` - use the verbose mode

The benchmark creates a number of threads specified by user and each of threads performs the corresponding insert/find/erase operations according to the passed percentage.
Calculations are repeated several times and the benchmark prints the median, mean, minimum and maximum time for a single calculation (in seconds).

*Note*: sum of passed insert, find and erase percentages should be equal to `100`.

## Possible output (verbose mode)

`./bench_unordered_map --verbose --insert 20 --find 80 --erase 20`

```
Testing tomkv::unordered_map
Info:
        Total number of threads = 88
        Number of threads for insertion = 17
        Number of threads for lookup = 70
        Number of threads for erasure = 0
        Number of elements = 1000
Elapsed time (median): 0.0123989
Elapsed time (mean): 0.0134467
Elapsed time (min): 0.0109872
Elapsed time (max): 0.0215485
```
