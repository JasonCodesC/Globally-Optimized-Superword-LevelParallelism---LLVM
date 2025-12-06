# Globally-Optimized-Superword-LevelParallelism---LLVM

## Overview

This Project is an implementation of the paper GoSLP, originally created by Charith Mendis and Saman Amarasinghe.
This was created as a final project for CSE 583 (Advanced Compilers) at the University of Michigan, Ann Arbor

GoSLP aims to take a more optimized approach of Superword Level Parallelism than the naive LLVM implementation.


## File Structure

```text
/project-root
│── /GoSLP
│   │   ├── CMakeLists.txt          # build entry for pass + bench
│   │   ├── /bench                  # bench.cpp, bench.sh
│   │   └── /GoSLPPass              # pass sources
│   │       ├── CandidatePacks.cpp
│   │       ├── Emit.cpp
│   │       ├── GoSLPPass.cpp
│   │       ├── ILP.cpp
│   │       ├── PermuteDP.cpp
│   │       ├── ShuffleCost.cpp
│   │       └── VecGraph.cpp
│── README.md
│── .gitignore
```

## Overview of GoSLP

GoSLP takes advantage of SIMD instructions. In LLVM we use greedy hueristics when we want to use vectorization which although comes with much less overhead than GoSLP, it is slower at runtime and we are loosing out on performance. GoSLP goes through a few different stages to improve on LLVM that will be described in order below.


## Pass Overview


### CandidatePacks.cpp:

Enumerates legal packs (isomorphic ops, no internal dependences, same block, adjacent loads/stores). Builds the CandidatePairs structure mapping each instruction to the packs it can join so the ILP can choose among them.

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

## Results: mean/median over five runs, ILP cap 60s (nested set to 800s)

| Kernel (iters) | Baseline mean s | Baseline median s | GoSLP mean s | GoSLP median s | Mean speedup | Median speedup |
|---|---|---|---|---|---|---|
| vec4_add (2e10) | 23.047046 | 23.102339 | 20.154191 | 20.820032 | ~12.6% | ~9.9% |
| saxpy3 (2e9) | 6.226196 | 6.179942 | 6.080081 | 6.024897 | ~2.35% | ~2.51% |
| dot64 (2e8) | 6.144559 | 6.392963 | 5.241463 | 5.371704 | ~14.7% | ~16.0% |
| nested (500) | 6.577738 | 6.555093 | 6.472756 | 6.470783 | ~1.6% | ~1.3% |

Checksums matched for all runs (Shows Program Correctness ).


## Authors
Jason Majoros  
Rishi Rallapalli  
Luke Nelson  
Anvita Gollu  
