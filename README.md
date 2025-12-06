# Globally-Optimized-Superword-LevelParallelism---LLVM

## Overview

This Project is an implementation of the paper GoSLP, originally created by Charith Mendis and Saman Amarasinghe.
This was created as a final project for CSE 583 (Advanced Compilers) at the University of Michigan, Ann Arbor

GoSLP aims to take a more optimized approach of Superword Level Parallelism than the naive LLVM implementation.


## File Structure

```text
/project-root
│── /GoSLP
│   │   ├── CMakeLists.txt          # For compilation
│   │   ├── run.sh                  # Script to run
│   │   ├── /tests                   # A folder of C programs for tests
│   │   ├── /testPass                # A folder for the test passes
│   │   ├── /GoSLPPass
│   │   │   ├── CandidatePacks.cpp      # Find Legal Packs
│   │   │   ├── CMakeLists.txt          # For compilation
│   │   │   ├── Emit.cpp                # Rewrite IR
│   │   │   ├── GoSLPPass.cpp           # Pass that brings all the logic together
│   │   │   ├── ILP.cpp                 # ILP to choose which packs to keep
│   │   │   ├── PermuteDP.cpp           # DP to pick lane orders
│   │   │   ├── ShuffleCost.cpp         # Shuffle Cost model
│   │   │   └── VecGraph.cpp            # Dependency Graph of Packs
│── README.md  # Documentation
│── .gitignore 

```

## Overview of GoSLP

GoSLP takes advantage of SIMD instructions. In LLVM we use greedy hueristics when we want to use vectorization which although comes with much less overhead than GoSLP, it is slower at runtime and we are loosing out on performance. GoSLP goes through a few different stages to improve on LLVM that will be described in order below.


## Pass Overview


### CandidatePacks.cpp:

In this file we find all of the candidate packs inside of a function. We follow the GoSLP paper pretty strictly and ensure that any candidate pack is isomorphic, has no dependences between the two, are in the same basic block, and access adjacent memory locations if they are load/store instructions. We also ensure that the two instructions are the same types, in this implimentation we are limited to integers, floating point numbers, and vector types (this plays into iterative widening). This file then returns a CandidatePairs object to the main pass which contains a vector of the instruction packs along with a hash map from each instruction to a vector of CandidateId objects which contains the width and index of the candidates. But all you really need to know is that this maps each instruction to a list of all the candidate packs that this instruction participates in so the ILP can find the best one.

### VecGraph.cpp:

Builds the dependence/conflict graph over candidate packs so mutually dependent packs are not chosen together.

### ShuffleCost.cpp:

Estimates pack/unpack/shuffle costs for each pack so the ILP can compare scalar vs vectorized cost.

### ILP.cpp:

Formulates and solves the GoSLP integer linear program using the pack costs and conflict graph to pick the best subset of packs.

### PermuteDP.cpp:

Dynamic-programming search for the best lane permutation per pack to minimize shuffle overhead.

### Emit.cpp:

Rewrites the IR: materializes vector ops for chosen packs, inserts shuffles/extracts, and erases the replaced scalar instructions.

### GoSLPPass.cpp:

Coordinates the full pipeline per function: collect candidates, build conflicts, estimate costs, solve the ILP, choose permutations, and emit vector IR.

## How to run

```bash
cd GoSLP
# Build the pass
cmake -B build -S . -DPASS_DIR=GoSLPPass
cmake --build build --target GoSLPPass

# Run a benchmark (vec4_add | saxpy3 | dot64 | quad_loops)
./bench/bench.sh vec4_add 200000000
```

Both baseline and GoSLP are compiled with `-O3 -fno-slp-vectorize -ffp-contract=off` to isolate this pass. Set `SKIP_BUILD=0` to rebuild the pass; use `SKIP_BUILD=1` for repeated timing runs.

## Results (mean/median over multiple runs, ILP cap 60s; nested 800s)

| Kernel (iters) | Baseline mean µs | Baseline median µs | GoSLP mean µs | GoSLP median µs | Mean speedup | Median speedup |
|---|---|---|---|---|---|---|
| vec4_add (2e10) | 23,047,046 | 23,102,339 | 20,154,191 | 20,820,032 | ~12.6% | ~9.9% |
| saxpy3 (2e9) | 6,226,196 | 6,179,942 | 6,080,081 | 6,024,897 | ~2.35% | ~2.51% |
| dot64 (2e8) | 6,144,559 | 6,392,963 | 5,241,463 | 5,371,704 | ~14.7% | ~16.0% |
| quad_loops (nested, 500) | 6,577,738 | 6,555,093 | 6,472,756 | 6,470,783 | ~1.6% | ~1.3% |

Checksums matched for all runs (Shows Program Correctness ).


## Authors
Jason Majoros  
Rishi Rallapalli  
Luke Nelson  
Anvita Gollu  
