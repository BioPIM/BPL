
# Mash

This project provides a benchmark for comparing sketches; a sketch is an ordered list of integers representing a kind of signature of a genomic sequence.

Comparing two genomic sequences can therefore be approximated by comparing two sketches, that is, two ordered sequences of integers of the same size N, which can be performed with a complexity of O(N). 

As a result, a comparison score is produced, making it possible to estimate the similarity between the two genomic sequences.

This benchmark creates databases of random sketches of size 10000 and compares them with different implementations:
* **MC std**: standard multicore 
* **MC BPL**: BPL with multicore architecture
* **PIM SDK**: UPMEM SDK 
* **PIM BPL**: BPL with UPMEM architecture

## How to build ?

You can build the BPL binaries as follows:

- create a 'build' directory and go into it
- run 'cmake ..'  A message will be displayed that will inform how to set the DPU_BINARIES_DIR environment variable
- run 'make'

In the same `build` directory, you can build the UPMEM SDK version with:

`dpu-upmem-dpurte-clang -O3 -DNR_TASKLETS=16 -DSTACK_SIZE_DEFAULT=4000 ../src/main/benchmark_sdk_pim.c -o benchmark_sdk_pim`

Then, you can launch the benchmark binary with `./src/main/mash > benchmark.txt` which will produce some traces in the `benchmark.txt` file. **Note:** this benchmark can take some time.

## Results output

In the `results` directory, there is a Jupyter notebook named `mash.ipynb` that will generate a TeX table from the `benchmark.txt` file.

