# Cache Simulator

## Description

This project is a lab for Architecture Design, PKU.
The simulator can simulates  multiple layer cache with self-defined configurations.




## Contents
This repo contains three version of Cache Simulator:
- [Simple Version](https://github.com/VegB/Cache-Simulator/tree/master/Simple)

  Contains the most basic components of a cache with LRU as replace policy. Run pseudo traces and calculates the miss rate and access time of each level.

- [Mixed Version](https://github.com/VegB/Cache-Simulator/tree/master/mixed)

  A cache-supported [RISCV-Simulator](https://github.com/VegB/RISCV-Simulator). 

  Takes in a C/C++ program as input and load code and data into cache each to simulate the execution process.

- [Optimized Version](https://github.com/VegB/Cache-Simulator/tree/master/Complex)

  An advanced cache implemented with the following optimization to lower AMAT:

  - Replace Policy: LRU/ LFU/ LIRS
  - Bypass Strategy
  - Prefetch Stragegy

