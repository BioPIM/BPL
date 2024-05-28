
# BPL: a C++ library that eases code development for Process In Memory architecture.

## Introduction

This project aims at designing a C++ library that eases Process In Memory development. The proposed API should provide abstractions that make PIM development similar to more conventional development (to an extent), for instance multithreading development. Unit and benchmark tests will be written during the library development.

As a gold standard, a multicore implementation will also be provided, which will make it possible to compare results for both PIM and multicore architectures.

A specific implementation of this library will be provided for the PIM architecture designed by UPMEM. In order to be sure of the behavior of the library, unit tests using the library and their conterpart using low-level UPMEM API will be written; the same thing could be done as well for benchmark tests.

## Build instructions

This project uses CMake so the conventional build instructions are as usual:
1. `mkdir build`
2. `cd build`
3. `cmake ..`

Then you can launch `make help` to know all available targets.

## Hello World !

From the `build` directory, you can run `make buildHelloWorld` in order to generate an archive named `helloworld.tar.gz` that will provide a standalone "Hello World" project using the library.

Once the project is unarchived, you can follow the instructions in the README file.

## Library overview

The project has the following directories structure:
- doc: documentation of the library (doxygen for the source code, slides explaining the library)
- export: holds the "Hello World" project
- scripts: a few scripts for day-by-day project management
- src: holds the source code for the library
- test: tests for the library (unit/functional/benchmark tests)

## Philosophy of the library 

Actually, the initial purpose of this library is to ease the development for the PIM architecture provided by [UPMEM](https://www.upmem.com/). UPMEM provides a [SDK](https://sdk.upmem.com/2023.1.0/index.html) implemented for different languages (C/C++, Java, Python).

From the developer's point of view, using the power of the PIM architecture through this SDK may be unfamiliar. In particular:
- low-level code for communication between the CPU and the PIM components has to be systematically written, which makes the code less readable and more error-prone
- two programs have to be written: one for the PIM part (the one that actually runs the targeted task) and one for the "Host" part that feeds the PIM components with the data to be processed by the targeted task

Since the underlying idea of using PIM architecture is to speedup some tasks, it would be interesting to provide a PIM programming model close to a multithreading approach that is well understood by the community. So, the BPL library should be able to:
1. hide as much as possible some tedious technical points described above
2. mimic a multithreading programming model

These two points reflects the way the library will be implemented: first, an attempt to hide the communication exchanges between the hosts and the PIM components will be designed and implemented. Then, tools for easing parallelization on PIM will be developped. Note that even if these two points are related, they can be addressed separately. 

**Important point:**  as for multicore architecture, PIM architecture provides a way to speedup algorithms. However, parallelizing a specific algorithm is not an easy task in general and there exists no automatic procedure to parallelize an algorithm. This means that  the BPL library will not automagically parallelize an algorithm. It can provide some tools to ease this parallelization for a PIM architecture but in the end, the developper will still have to think how design her/his algorithm to make it work on an architecture allowing parallelization such as PIM or multicore.





