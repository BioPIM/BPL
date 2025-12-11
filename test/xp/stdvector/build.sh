#!/usr/bin/bash

rm dpu host

clang++ dpu.cpp -o dpu  -DDPU \
    --target=dpu-upmem-dpurte  -std=c++20 -fno-exceptions -fno-rtti  -D__x86_64__ \
    -DNR_TASKLETS=16 -O3 -I/opt/gcc/14.2.0/include/c++/14.2.0 -I/opt/gcc/14.2.0/include/c++/14.2.0/x86_64-pc-linux-gnu -I/opt/gcc/14.2.0/include/c++/14.2.0/backward -I/opt/gcc/14.2.0/lib/gcc/x86_64-pc-linux-gnu/14.2.0/include -I/usr/local/include -I/opt/gcc/14.2.0/include -I/opt/gcc/14.2.0/lib/gcc/x86_64-pc-linux-gnu/14.2.0/include-fixed/x86_64-linux-gnu -I/opt/gcc/14.2.0/lib/gcc/x86_64-pc-linux-gnu/14.2.0/include-fixed -I/usr/include/x86_64-linux-gnu -I/usr/include

g++ host.cpp -DNR_TASKLETS=16  `dpu-pkg-config --cflags --libs dpu` -o host
