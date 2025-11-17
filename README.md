# Globally-Optimized-Superword-LevelParallelism---LLVM


## Overview

This Project is an implementation of the paper GoSLP, originally created by Charith Mendis and Saman Amarasinghe.
This implementation was created as a final project for CSE 583 (Advanced Compilers) at the University of Michigan, Ann Arbor

GoSLP aims to take a more optimized approach of Superword Level Parallelism than the naive LLVM implementation.


## File Structure

```text
/project-root
│── /src
│   ├── /cpp
│   │   ├── CandidatePacks.cpp      # Find Legal Packs
│   │   ├── Emit.cpp                # Rewrite IR
│   │   ├── GoSLPPass.cpp           # Pass that brings all the logic together
│   │   ├── ILP.cpp                 # ILP to choose which packs to keep
│   │   ├── PermuteDP.cpp           # DP to pick lane orders
│   │   ├── ShuffleCost.cpp         # Shuffle Cost model
│   │   └── VecGraph.cpp            # Dependency Graph of Packs
│── README.md  # Documentation

```

# TODO explain how it works

# TODO explain actual coding and how to run


## Authors
Jason Majoros  
Rishi Rallapalli  
Luke Nelson  
Anvita Gollu  
