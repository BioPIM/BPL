
# Checksum

This project provides the minimal material for building and launching a small program on DPU with the help of the BPL library.

## Source code

The source code for this project is in the 'src' directory:

- main.cpp holds the main function and shows how to configure and use a launcher
- tasks/Checksum.hpp: structure providing the algorithm to be executed by the launcher

## How to build ?

- create a 'build' directory and go into it
- run 'cmake ..'  A message will be displayed that will inform how to set the DPU_BINARIES_DIR environment variable
- run 'make'

Then you can launch the Host binary with './src/main/checksum'
