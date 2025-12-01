#pragma once
#include <vector>
#include <unordered_set>
#include <chrono>
#include <functional>
#include "CandidatePacks.hpp"

std::vector<bool> solveILP(const CandidatePairs &C,
                           const std::vector<double> &PackCost,
                           double TimeLimitSeconds);