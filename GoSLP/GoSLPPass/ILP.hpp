#pragma once
#include <vector>
#include <unordered_set>
#include <chrono>
#include <functional>
#include "CandidatePacks.hpp"

struct ILPModel {
  // Vector savings term (negative means profitable).
  std::vector<double> VecSavings;
  // Packing cost for candidate vector packs.
  std::vector<double> PackCost;
  // Packing cost for non-vector packs (indexed by CandidatePairs::NonVecPacks).
  std::vector<double> NonVecPackCost;
  // Lane extraction cost for each candidate pack.
  std::vector<std::vector<double>> LaneExtractCost;
};

std::vector<bool> solveILP(const CandidatePairs &C, const ILPModel &Model,
                           double TimeLimitSeconds);
