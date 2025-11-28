
#include <vector>
#include <unordered_set>
#include <chrono>
#include <functional>

// Solve a 0/1 "ILP" for goSLP pack selection:
//
//   minimize   sum_i Cost[i] * x_i
//   subject to for every scalar instruction S:
//                sum_{packs P containing S} x_P <= 1
//   x_i ∈ {0,1}
//
// We assume CandidatePairs is already defined (from CandidatePacks.cpp)
// and visible because this file is #included after it.

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

  // Precompute "best possible improvement" suffix:
  // suffixNeg[i] = sum of all negative costs Cost[j] for j >= i.
  // This is a lower bound on how much more we can decrease the objective
  // if from position i onward we pick every beneficial (negative-cost) pack.
  std::vector<double> suffixNeg(N + 1, 0.0);
  for (int i = N - 1; i >= 0; --i) {
    double add = (Cost[i] < 0.0) ? Cost[i] : 0.0;
    suffixNeg[i] = suffixNeg[i + 1] + add;
  }

  // Track which scalar instructions are already used by chosen packs,
  // to enforce "each instruction appears in at most one pack".
  std::unordered_set<const Instruction *> usedInsts;

  // Baseline: choosing no packs costs 0.
  double bestCost = 0.0;
  double curCost = 0.0;

  // Time limit
  auto start = std::chrono::steady_clock::now();
  auto limit = start + std::chrono::duration<double>(TimeLimitSeconds);
  bool timeLimitHit = false;

  std::function<void(int)> dfs = [&](int idx) {
    if (timeLimitHit)
      return;

    auto now = std::chrono::steady_clock::now();
    if (now > limit) {
      timeLimitHit = true;
      return;
    }

    if (idx == N) {
      // Reached a leaf: update best solution if strictly better.
      if (curCost < bestCost) {
        bestCost = curCost;
        best = cur;
      }
      return;
    }

    // Lower bound: current cost + "best possible" improvement from remaining
    // negative-cost packs. If even in the best case we can't beat bestCost,
    // we can safely prune this branch.
    double lowerBound = curCost + suffixNeg[idx];
    if (lowerBound >= bestCost)
      return;

    // Option 1: skip this pack
    cur[idx] = false;
    dfs(idx + 1);

    // Option 2: try to take this pack, if it doesn't conflict
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
        if (res.second)  // this instruction wasn't present before
          newlyInserted.push_back(I);
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
