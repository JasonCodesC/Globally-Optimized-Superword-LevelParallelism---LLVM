#include "ILP.hpp"
#include <algorithm>
#include <limits>
#include <cmath>
#include <iostream>
#include <numeric>


#include <unordered_set>
#include <chrono>
#include <functional>
#include <limits>

using namespace llvm;

// Branch-and-bound over packs with "each scalar in at most one pack" constraint.
std::vector<bool> solveILP(const CandidatePairs &C,
                           const std::vector<double> &PackCost,
                           double TimeLimitSeconds) {
  const int N = static_cast<int>(C.Packs.size());
  std::vector<bool> best(N, false);
  std::vector<bool> cur(N, false);

  if (N == 0)
    return best;

  // Normalize costs to length N (missing entries treated as 0).
  std::vector<double> Cost(N, 0.0);
  for (int i = 0; i < N; ++i) {
    if (i < static_cast<int>(PackCost.size()))
      Cost[i] = PackCost[i];
    else
      Cost[i] = 0.0;
  }

  // suffixNeg[i] = sum of all negative costs Cost[j] for j >= i.
  // This gives a lower bound on how much we can further reduce the
  // objective if from position i onward we pick every beneficial pack.
  std::vector<double> suffixNeg(N + 1, 0.0);
  for (int i = N - 1; i >= 0; --i) {
    double add = (Cost[i] < 0.0) ? Cost[i] : 0.0;
    suffixNeg[i] = suffixNeg[i + 1] + add;
  }

  // Track which scalar instructions are already used by chosen packs.
  std::unordered_set<const Instruction *> usedInsts;

  // Cheap greedy seed so we always have a feasible (often good) solution even
  // if the time limit is hit before exploring the full tree.
  std::vector<bool> greedy(N, false);
  std::unordered_set<const Instruction *> greedyUsed;
  std::vector<int> order(N);
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(),
                   [&](int A, int B) { return Cost[A] < Cost[B]; });

  double bestCost = 0.0;
  for (int idx : order) {
    if (Cost[idx] >= 0.0)
      break;
    bool conflict = false;
    for (const Instruction *I : C.Packs[idx]) {
      if (greedyUsed.count(I)) {
        conflict = true;
        break;
      }
    }
    if (!conflict) {
      greedy[idx] = true;
      bestCost += Cost[idx];
      for (const Instruction *I : C.Packs[idx])
        greedyUsed.insert(I);
    }
  }
  best = greedy;

  double curCost = 0.0;

  auto start = std::chrono::steady_clock::now();
  auto limit = start + std::chrono::duration<double>(TimeLimitSeconds);
  bool timeLimitHit = false;

  std::function<void(int)> dfs = [&](int idx) {
    if (timeLimitHit)
      return;

    if (TimeLimitSeconds > 0.0) {
      auto now = std::chrono::steady_clock::now();
      if (now > limit) {
        timeLimitHit = true;
        return;
      }
    }

    if (idx == N) {
      // Leaf: update best solution if strictly better.
      if (curCost < bestCost) {
        bestCost = curCost;
        best = cur;
      }
      return;
    }

    // Lower bound on achievable cost from this node:
    double lowerBound = curCost + suffixNeg[idx];
    if (lowerBound > bestCost) {
      // Even in best-case (taking all remaining neg-cost packs) we can't beat
      // the current best → prune this branch.
      return;
    }

    // Option 1: skip this pack
    cur[idx] = false;
    dfs(idx + 1);

    // Option 2: try taking this pack, if it doesn't conflict
    bool conflict = false;
    const auto &packInsts = C.Packs[idx];
    for (const Instruction *I : packInsts) {
      if (usedInsts.count(I)) {
        conflict = true;
        break;
      }
    }

    if (!conflict) {
      cur[idx] = true;
      double oldCost = curCost;
      curCost += Cost[idx];

      // Insert its instructions into usedInsts and remember which were new
      std::vector<const Instruction *> newlyInserted;
      newlyInserted.reserve(packInsts.size());
      for (const Instruction *I : packInsts) {
        auto res = usedInsts.insert(I);
        if (res.second) {
          newlyInserted.push_back(I);
        }
      }

      dfs(idx + 1);

      // Backtrack
      for (const Instruction *I : newlyInserted) {
        usedInsts.erase(I);
      }
      curCost = oldCost;
      cur[idx] = false;
    }
  };

  dfs(0);
  return best;
}
