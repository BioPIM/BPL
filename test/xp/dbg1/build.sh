#!/usr/bin/bash

rm dpu host

dpu-upmem-dpurte-clang -DNR_TASKLETS=16 -O3 dpu.c -o dpu
gcc host.c -DNR_TASKLETS=16  `dpu-pkg-config --cflags --libs dpu` -o host
