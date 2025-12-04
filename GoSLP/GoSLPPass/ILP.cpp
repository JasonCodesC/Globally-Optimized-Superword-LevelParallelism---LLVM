#include "ILP.hpp"
#include <algorithm>
#include <limits>
#include <cmath>
#include <iostream>
#include <numeric>

// Solve a 0/1 "ILP" for goSLP pack selection with full debugging

// std::vector<bool> solveILP(const CandidatePairs &C,
//                            const std::vector<double> &PackCost,
//                            double TimeLimitSeconds) {
//   const int N = static_cast<int>(C.Packs.size());
  
//   std::cerr << "=== ILP Solver Debug ===" << std::endl;
//   std::cerr << "Number of packs: " << N << std::endl;
//   std::cerr << "Number of costs: " << PackCost.size() << std::endl;
  
//   // Handle edge cases
//   if (N == 0) {
//     std::cerr << "No packs to process!" << std::endl;
//     return std::vector<bool>();
//   }
  
//   // Normalize costs to length N
//   std::vector<double> Cost(N, 0.0);
//   int negativeCosts = 0;
//   int positiveCosts = 0;
//   int zeroCosts = 0;
  
//   for (int i = 0; i < N; ++i) {
//     if (i < static_cast<int>(PackCost.size())) {
//       Cost[i] = PackCost[i];
//     }
//     if (Cost[i] < -1e-9) negativeCosts++;
//     else if (Cost[i] > 1e-9) positiveCosts++;
//     else zeroCosts++;
//   }
  
//   std::cerr << "Costs: " << negativeCosts << " negative, " 
//             << positiveCosts << " positive, " << zeroCosts << " zero" << std::endl;
  
//   // Print first few costs
//   std::cerr << "First 10 costs: ";
//   for (int i = 0; i < std::min(10, N); ++i) {
//     std::cerr << Cost[i] << " ";
//   }
//   std::cerr << std::endl;
  
//   // Check pack sizes
//   int totalInstructions = 0;
//   int emptyPacks = 0;
//   for (int i = 0; i < N; ++i) {
//     if (C.Packs[i].empty()) {
//       emptyPacks++;
//     }
//     totalInstructions += C.Packs[i].size();
//   }
//   std::cerr << "Empty packs: " << emptyPacks << std::endl;
//   std::cerr << "Total instruction references: " << totalInstructions << std::endl;
//   std::cerr << "Avg pack size: " << (N > 0 ? (double)totalInstructions / N : 0) << std::endl;
  
//   // Build conflict graph
//   std::unordered_map<const Instruction*, std::vector<int>> instToPacks;
//   for (int i = 0; i < N; ++i) {
//     for (const Instruction *I : C.Packs[i]) {
//       instToPacks[I].push_back(i);
//     }
//   }
  
//   std::cerr << "Unique instructions: " << instToPacks.size() << std::endl;
  
//   // Check for heavily conflicting instructions
//   int maxConflicts = 0;
//   for (const auto &[inst, packs] : instToPacks) {
//     if (packs.size() > maxConflicts) {
//       maxConflicts = packs.size();
//     }
//   }
//   std::cerr << "Max packs sharing one instruction: " << maxConflicts << std::endl;
  
//   // Initialize solution
//   std::vector<bool> bestSol(N, false);
//   double bestCost = 0.0;
  
//   std::cerr << "\n=== Greedy Phase ===" << std::endl;
  
//   // Greedy initialization
//   {
//     std::unordered_set<const Instruction*> used;
//     std::vector<bool> greedySol(N, false);
//     double greedyCost = 0.0;
    
//     // Sort packs by cost
//     std::vector<int> indices(N);
//     std::iota(indices.begin(), indices.end(), 0);
//     std::sort(indices.begin(), indices.end(), [&](int a, int b) {
//       return Cost[a] < Cost[b];
//     });
    
//     int greedyTaken = 0;
//     int greedySkipped = 0;
    
//     for (int idx : indices) {
//       if (Cost[idx] >= -1e-9) {
//         std::cerr << "Greedy: Stopping at cost " << Cost[idx] << std::endl;
//         break;
//       }
      
//       bool conflict = false;
//       for (const Instruction *I : C.Packs[idx]) {
//         if (used.count(I)) {
//           conflict = true;
//           break;
//         }
//       }
      
//       if (!conflict) {
//         greedySol[idx] = true;
//         greedyCost += Cost[idx];
//         greedyTaken++;
//         for (const Instruction *I : C.Packs[idx]) {
//           used.insert(I);
//         }
//         std::cerr << "Greedy: TOOK pack " << idx << " (cost=" << Cost[idx] << ")" << std::endl;
//       } else {
//         greedySkipped++;
//       }
//     }
    
//     std::cerr << "Greedy result: took " << greedyTaken << ", skipped " << greedySkipped 
//               << ", cost=" << greedyCost << std::endl;
    
//     if (greedyCost < bestCost - 1e-9) {
//       bestCost = greedyCost;
//       bestSol = greedySol;
//       std::cerr << "Greedy improved! New best cost: " << bestCost << std::endl;
//     }
//   }
  
//   std::cerr << "\n=== Branch-and-Bound Phase ===" << std::endl;
  
//   // Sort packs for branch-and-bound
//   std::vector<int> order(N);
//   std::iota(order.begin(), order.end(), 0);
//   std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
//     if (std::abs(Cost[a] - Cost[b]) > 1e-9)
//       return Cost[a] < Cost[b];
//     return C.Packs[a].size() < C.Packs[b].size();
//   });
  
//   // Compute suffix bounds
//   std::vector<double> suffixMin(N + 1, 0.0);
//   for (int i = N - 1; i >= 0; --i) {
//     int packIdx = order[i];
//     double addMin = std::min(0.0, Cost[packIdx]);
//     suffixMin[i] = suffixMin[i + 1] + addMin;
//   }
  
//   std::cerr << "Suffix bound at root: " << suffixMin[0] << std::endl;
  
//   // Branch-and-bound state
//   std::vector<bool> curSol(N, false);
//   std::unordered_set<const Instruction*> usedInsts;
//   double curCost = 0.0;
  
//   auto startTime = std::chrono::steady_clock::now();
//   auto timeLimit = startTime + std::chrono::duration<double>(TimeLimitSeconds);
//   bool timeLimitHit = false;
  
//   uint64_t nodesExplored = 0;
//   uint64_t nodesPruned = 0;
//   uint64_t solutionsFound = 0;
  
//   std::function<void(int)> branchAndBound = [&](int pos) {
//     if ((++nodesExplored & 0x3FFF) == 0) {
//       if (std::chrono::steady_clock::now() >= timeLimit) {
//         timeLimitHit = true;
//         return;
//       }
//     }
    
//     if (timeLimitHit)
//       return;
    
//     if (pos == N) {
//       solutionsFound++;
//       if (curCost < bestCost - 1e-9) {
//         std::cerr << "Found better solution! Cost: " << curCost 
//                   << " (prev: " << bestCost << ")" << std::endl;
//         bestCost = curCost;
//         for (int i = 0; i < N; ++i) {
//           bestSol[order[i]] = curSol[i];
//         }
//       }
//       return;
//     }
    
//     double lowerBound = curCost + suffixMin[pos];
//     if (lowerBound >= bestCost - 1e-9) {
//       ++nodesPruned;
//       return;
//     }
    
//     int packIdx = order[pos];
//     const auto &packInsts = C.Packs[packIdx];
    
//     bool hasConflict = false;
//     for (const Instruction *I : packInsts) {
//       if (usedInsts.count(I)) {
//         hasConflict = true;
//         break;
//       }
//     }
    
//     bool takeBranchFirst = (Cost[packIdx] < -1e-9) && !hasConflict;
    
//     if (takeBranchFirst) {
//       // Try TAKE first
//       curSol[pos] = true;
//       double oldCost = curCost;
//       curCost += Cost[packIdx];
      
//       std::vector<const Instruction*> addedInsts;
//       for (const Instruction *I : packInsts) {
//         if (usedInsts.insert(I).second) {
//           addedInsts.push_back(I);
//         }
//       }
      
//       branchAndBound(pos + 1);
      
//       for (const Instruction *I : addedInsts) {
//         usedInsts.erase(I);
//       }
//       curCost = oldCost;
//       curSol[pos] = false;
      
//       // Try SKIP
//       branchAndBound(pos + 1);
      
//     } else {
//       // Try SKIP first
//       curSol[pos] = false;
//       branchAndBound(pos + 1);
      
//       if (!hasConflict) {
//         curSol[pos] = true;
//         double oldCost = curCost;
//         curCost += Cost[packIdx];
        
//         std::vector<const Instruction*> addedInsts;
//         for (const Instruction *I : packInsts) {
//           if (usedInsts.insert(I).second) {
//             addedInsts.push_back(I);
//           }
//         }
        
//         branchAndBound(pos + 1);
        
//         for (const Instruction *I : addedInsts) {
//           usedInsts.erase(I);
//         }
//         curCost = oldCost;
//         curSol[pos] = false;
//       }
//     }
//   };
  
//   branchAndBound(0);
  
//   std::cerr << "\n=== Results ===" << std::endl;
//   std::cerr << "Nodes explored: " << nodesExplored << std::endl;
//   std::cerr << "Nodes pruned: " << nodesPruned << std::endl;
//   std::cerr << "Solutions found: " << solutionsFound << std::endl;
//   std::cerr << "Time limit hit: " << (timeLimitHit ? "YES" : "NO") << std::endl;
//   std::cerr << "Best cost: " << bestCost << std::endl;
  
//   int selectedPacks = 0;
//   for (int i = 0; i < N; ++i) {
//     if (bestSol[i]) {
//       selectedPacks++;
//       std::cerr << "Selected pack " << i << " (cost=" << Cost[i] << ")" << std::endl;
//     }
//   }
//   std::cerr << "Total selected: " << selectedPacks << "/" << N << std::endl;
//   std::cerr << "===================" << std::endl;
  
//   return bestSol;
// }



// ILP.cpp
#include "ILP.hpp"

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

  // Start with +inf so first leaf sets the baseline.
  double bestCost = std::numeric_limits<double>::infinity();
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
