# bench_storage performance benchmark

`bench_storage` is a performance benchmark for `tomkv::storage` component. It allows setting the percentage of mount, read, write and insert operations as well as the number of threads and the number of operations per each thread.

## Command line options

`bench_storage` supports the following command line options:

- `--help` - prints help message with possible command line options
- `--mount <value>` (mandatory) - the percentage of threads that mounts new paths into the storage
- `--read <value>` (mandatory) - the percentage of threads that reads key-value pairs from the storage
- `--write <value>` (mandatory) - the percentage of threads that modifies key-value pairs in the storage
- `--insert <value>` (mandatory) - the percentage of threads that inserts new key-value pairs into the storage.
- `--num-threads <value>` (optional) - the number of threads to use while benchmarking. The default value is the hardware concurrency of the current system.
- `--num-operations <value>` (optional) - the number of operations that each thread will perform on the storage.
- `--verbose` - use verbose mode.

The benchmark creates a number of threads specified by user and each of threads performs the corresponding mount/read/write/insert operations according to the passed percentage.
Calculations are repeated several times and the benchmark prints the median, mean, minimum and maximum time for single calculation (in seconds).

*Note*: sum of passed mount, read, write and insert percentages should be equal to `100`.

## Possible output (verbose mode)

`./bench_storage --verbose --mount 25 --read 25 --write 25 --insert 25`

```
Info:
        Total number of threads = 88
        Number of threads for mounting = 22
        Number of threads for reading = 22
        Number of threads for writing = 22
        Number of threads for inserting = 22
        Number of operations per thread = 10
Elapsed time (median): 2.37192
Elapsed time (mean): 2.29594
Elapsed time (min): 1.92479
Elapsed time (max): 2.62607
```
