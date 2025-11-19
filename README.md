# Globally-Optimized-Superword-LevelParallelism---LLVM

Luke (VecGraph) --> Rishi (ShuffleCost) --> Anvita (PermuteDP) and Jason (Emit, GoSLPPass, ILP)

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

## Overview of GoSLP


## File Overview


### CandidatePacks.cpp:

In this file we find all of the candidate packs inside of a function. We follow the GoSLP paper pretty strictly and ensure that any candidate pack is isomorphic, has no dependences between the two, are in the same basic block, and access adjacent memory locations if they are load/store instructions. We also ensure that the two instructions are the same types, in this implimentation we are limited to integers, floating point numbers, and vector types (this plays into iterative widening). This file then returns a CandidatePairs object to the main pass which contains a vector of the instruction packs along with a hash map from each instruction to a vector of CandidateId objects which contains the width and index of the candidates. But all you really need to know is that this maps each instruction to a list of all the candidate packs that this instruction participates in so the ILP can find the best one.

### VecGraph.cpp:

### ShuffleCost.cpp:

### ILP.cpp:

### PermuteDP.cpp:

### Emit.cpp:


# TODO how to run


## Authors
Jason Majoros  
Rishi Rallapalli  
Luke Nelson  
Anvita Gollu  
