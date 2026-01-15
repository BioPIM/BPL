
# BPL: A C++ library that simplifies code development for Process-in-Memory architectures.

## Introduction

This project aims to design a C++ library that simplifies development for Process-in-Memory (PIM) architectures. The proposed API will provide abstractions that make PIM programming more similar—up to a certain extent—to conventional development paradigms, such as multithreaded programming. Unit tests and benchmark tests will be developed alongside the library.

As a reference baseline, a multicore implementation will also be provided, enabling performance and result comparisons between PIM and multicore architectures.

A specific implementation of this library will target the PIM architecture designed by UPMEM. To ensure the correctness of the library’s behavior, unit tests implemented using the library will be compared against equivalent implementations using the low-level UPMEM API. The same approach may also be applied to benchmark tests.

## Build instructions

This project uses CMake, so the build instructions follow standard conventions:
1. `mkdir build`
2. `cd build`
3. `cmake ..`

You can then run `make help` to list all available targets.

## Hello World !

From the `build` directory, you can run `make buildHelloWorld` to generate an archive named `helloworld.tar.gz` , which provides a standalone “Hello World” project using the library.

Once the archive is extracted, you can follow the instructions in the README file.

## Library overview

The project has the following directory structure:
- **doc**: Library documentation (Doxygen-generated source documentation, slides explaining the library)
- **export**: Contains the “Hello World” project
- **scripts**: Scripts for day-to-day project management
- **src**: Source code of the library
- **test**: Library tests (unit, functional, and benchmark tests)

## Philosophy of the library 

The initial purpose of this library is to simplify development for the Process-in-Memory (PIM) architecture provided by [UPMEM](https://www.upmem.com/). 
UPMEM offers an [SDK](https://sdk.upmem.com/2023.1.0/index.html) for several programming languages, including C/C++, Java, and Python.

From a developer’s perspective, leveraging the capabilities of the PIM architecture through this SDK can be unfamiliar and challenging. In particular:
- Low-level code for communication between the CPU and the PIM components must be written explicitly, which reduces code readability and increases the likelihood of errors.
- Two separate programs are required: one for the PIM side (which executes the target computation) and one for the host side, which supplies the PIM components with the data to be processed.

Since the main motivation for using PIM architectures is to accelerate certain workloads, it is desirable to offer a PIM programming model that is closer to the well-understood multithreading paradigm. Accordingly, the BPL library aims to:
1. Hide, as much as possible, the tedious technical aspects described above.
2. Mimic a multithreading programming model.

These goals shape the library’s implementation strategy. First, mechanisms will be designed and implemented to abstract and hide the communication between the host and the PIM components. Then, higher-level tools to simplify parallelization on PIM architectures will be developed. Although these two aspects are related, they can be addressed independently.

**Important point:**  As with multicore architectures, PIM architectures provide a means to accelerate algorithms. However, parallelizing a specific algorithm is generally a difficult task, and there is no automatic procedure to parallelize an arbitrary algorithm. Consequently, the BPL library will not automatically parallelize algorithms.
Instead, it provides tools to facilitate parallelization on PIM architectures. Ultimately, the developer must still design the algorithm with parallel execution in mind, adapting it to architectures that support parallelism, such as PIM or multicore systems.

## Example

We will now compare the BPL with the use of the UPMEM SDK on a simple example of vector checksum computation.

### Using the UPMEM SDK

The example is taken from the official UPMEM SDK documentation. Here is the [the DPU part](https://sdk.upmem.com/2023.2.0/032_DPURuntimeService_HostCommunication.html#memory-interface) (with all comments removed) that 
implements the algorithm:

```c
#include <mram.h>
#include <stdbool.h>
#include <stdint.h>
#define CACHE_SIZE 256
#define BUFFER_SIZE (1 << 16)
__mram_noinit uint8_t buffer[BUFFER_SIZE];
__host uint32_t checksum;
int main() {
  __dma_aligned uint8_t local_cache[CACHE_SIZE];
  checksum = 0;
  for (unsigned int bytes_read = 0; bytes_read < BUFFER_SIZE;) {
    mram_read(&buffer[bytes_read], local_cache, CACHE_SIZE);
    for (unsigned int byte_index = 0; (byte_index < CACHE_SIZE) 
         && (bytes_read < BUFFER_SIZE); byte_index++, bytes_read++) {
      checksum += (uint32_t)local_cache[byte_index];
    }
  }
  return checksum;
}
```
and [the host part](https://sdk.upmem.com/2023.2.0/032_DPURuntimeService_HostCommunication.html#rank-transfer-interface)

```c
#include <dpu.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#ifndef DPU_BINARY
#define DPU_BINARY "trivial_checksum_example"
#endif
#define BUFFER_SIZE (1 << 16)
void populate_mram(struct dpu_set_t set, uint32_t nr_dpus) {
  struct dpu_set_t dpu;
  uint32_t each_dpu;
  uint8_t *buffer = malloc(BUFFER_SIZE * nr_dpus);
  DPU_FOREACH(set, dpu, each_dpu) {
    for (int byte_index = 0; byte_index < BUFFER_SIZE; byte_index++) {
      buffer[each_dpu * BUFFER_SIZE + byte_index] = (uint8_t)byte_index;
    }
    buffer[each_dpu * BUFFER_SIZE] += each_dpu; 
    DPU_ASSERT(dpu_prepare_xfer(dpu, &buffer[each_dpu * BUFFER_SIZE]));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "buffer", 0, BUFFER_SIZE, 
  	DPU_XFER_DEFAULT));
  free(buffer);
}
void print_checksums(struct dpu_set_t set, uint32_t nr_dpus) {
  struct dpu_set_t dpu;
  uint32_t each_dpu;
  uint32_t checksums[nr_dpus];
  DPU_FOREACH(set, dpu, each_dpu) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, &checksums[each_dpu]));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "checksum", 0, 
  	sizeof(uint32_t), DPU_XFER_DEFAULT));
  DPU_FOREACH(set, dpu, each_dpu) {
    printf("[%u] checksum = 0x%08x\n", each_dpu, checksums[each_dpu]);
  }
}
int main() {
  struct dpu_set_t set, dpu;
  DPU_ASSERT(dpu_alloc(DPU_ALLOCATE_ALL, NULL, &set));
  DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
  DPU_ASSERT(dpu_get_nr_dpus(set, &nr_dpus));
  populate_mram(set, nr_dpus);
  DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
  print_checksums(set, nr_dpus);
  DPU_ASSERT(dpu_free(set));
  return 0;
}
```

### Using the BPL

Now, using the BPL API, the source code for the algorithm is:

```c++
#include <bpl/core/Task.hpp>
template<class ARCH>
struct VectorChecksum  {
    USING(ARCH);
    auto operator() (vector<uint32_t> const& v)  {
        return accumulate(v.begin(), v.end(), uint64_t{0});
    }
    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
```
and the way to use it:

```c++
#include <vector>
#include <numeric>
#include <iostream>
#include <bpl/bpl.hpp>
#include <tasks/VectorChecksum.hpp>
int main() {
    std::vector<uint32_t> v(1<<16);  
    std::iota (std::begin(v), std::end(v), 1);
    bpl::Launcher<bpl::ArchUpmem> launcher {1_dpu};
    std::cout << "checksum: " << launcher.run<VectorChecksum>(split(v)) << "\n";
}
```

A few remarks:

1. we can see a significant reduction in code size with the BPL;
2. moreover, code written using the BPL is also much more readable than with the UPMEM SDK. Indeed, reading the BPL code makes it immediately clear that the task is to compute the checksum of a vector, which is far less obvious in the UPMEM SDK version, 
where all the low-level SDK calls reduce overall readability;
3. finally, it should be noted that using the BPL makes it possible to adopt a traditional C++ style, in particular through the use of the standard library.

In addition, it is possible with the BPL to run our algorithm on a different architecture.
If one wishes to use a multicore architecture, it suffices to replace the previous definition of the launcher with:

```c++
bpl::Launcher<bpl::ArchMulticore> launcher {16_thread};
```

On the other hand, source code written using the UPMEM SDK would be of no use for execution on a multicore architecture, requiring a complete rewrite.