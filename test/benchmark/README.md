
# Benchmarking the BPL

## Build instructions

This project uses CMake, so the build instructions follow standard conventions:
1. `mkdir build`
2. `cd build`
3. `cmake ..`

You can then run `make help` to list all available targets.

## Launching the benchmark

From the `build` directory, you can run the benchmark with `./test/benchmark/host/bpl-benchmark.host`. 
**Note**: the execution may take a while.

This will produce traces that can be redirected in a `benchmark.txt` file. This file can then
be used in a Jupyter notebook named `1.Benchmark.ipynb` (in directory `test/benchmark/notebooks`) that will generate a benchmark report as a pdf file named [1.Benchmark.pdf](./notebooks/1.Benchmark.pdf).

It is also possible to generate an HTML output; the result file will be named [1.Benchmark.html](./notebooks/1.Benchmark.html)