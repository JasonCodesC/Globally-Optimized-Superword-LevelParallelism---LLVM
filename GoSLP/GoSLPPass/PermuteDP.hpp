#pragma once
#include "VecGraph.hpp"
#include "ShuffleCost.hpp"
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <queue>
#include <vector>
#include "llvm/IR/Instruction.h"

using namespace llvm;

using Permutation = std::vector<unsigned>;
using PermsList =  std::vector<Permutation>;
using CostVec = std::vector<InstructionCost>;
using Perms = std::unordered_map<int, Permutation>; // key = PackIdx(int), value = chosen lane ordering

PermsList generatePerms(unsigned width);
Perms choosePermutationsDP(const VecGraph &G, const std::vector<bool> &Chosen,
                           ShuffleCost &SC);
