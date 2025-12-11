#!/usr/bin/bash

./benchmark.py  results/write.uint8_t.csv       "write (uint8_t)"
./benchmark.py  results/write.uint16_t.csv      "write (uint16_t)"
./benchmark.py  results/write.uint32_t.csv      "write (uint32_t)"
./benchmark.py  results/write.uint64_t.csv      "write (uint64_t)"
./benchmark.py  results/iterate.uint32_t.csv    "iterate (uint32_t)"

