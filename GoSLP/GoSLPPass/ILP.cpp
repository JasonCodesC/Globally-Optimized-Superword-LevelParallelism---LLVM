#include "ILP.hpp"

#include <algorithm>
#include <numeric>
#include <unordered_set>

using namespace llvm;

namespace {

static bool hasChosenUse(const std::vector<uint32_t> &Uses,
                         const std::vector<bool> &Chosen) {
  for (uint32_t U : Uses) {
    if (U < Chosen.size() && Chosen[U])
      return true;
  }
  return false;
}

static double evaluateObjective(const CandidatePairs &C, const ILPModel &Model,
                                const std::vector<bool> &Chosen) {
  const size_t N = C.Packs.size();
  double Obj = 0.0;

  // VS term
  for (size_t P = 0; P < N; ++P) {
    if (Chosen[P] && P < Model.VecSavings.size())
      Obj += Model.VecSavings[P];
  }

  // PCvec term: pack candidate vectors when producer is not selected but some
  // vectorized use requires it.
  for (size_t P = 0; P < N; ++P) {
    if (Chosen[P])
      continue;

    auto It = C.VecVecUses.find(static_cast<uint32_t>(P));
    if (It == C.VecVecUses.end())
      continue;

    if (hasChosenUse(It->second, Chosen) && P < Model.PackCost.size())
      Obj += Model.PackCost[P];
  }

  // PCnonvec term.
  for (auto &Entry : C.NonVecVecUses) {
    uint32_t NonVecIdx = Entry.first;
    if (!hasChosenUse(Entry.second, Chosen))
      continue;
    if (NonVecIdx < Model.NonVecPackCost.size())
      Obj += Model.NonVecPackCost[NonVecIdx];
  }

  // UC term: extract once per lane if any use remains non-vectorized.
  for (size_t P = 0; P < N; ++P) {
    if (!Chosen[P])
      continue;

    if (P >= C.LaneUses.size() || P >= Model.LaneExtractCost.size())
      continue;

    const auto &Lanes = C.LaneUses[P];
    const auto &LaneCosts = Model.LaneExtractCost[P];

    for (size_t Lane = 0; Lane < Lanes.size() && Lane < LaneCosts.size();
         ++Lane) {
      const LaneUseInfo &Info = Lanes[Lane];
      bool NeedExtract = Info.HasOutsideUse;

      if (!NeedExtract) {
        for (const auto &UserEntry : Info.UserToVectorUses) {
          const auto &UsePacks = UserEntry.second;
          bool UserVectorized = false;
          for (uint32_t UsePack : UsePacks) {
            if (UsePack < Chosen.size() && Chosen[UsePack]) {
              UserVectorized = true;
              break;
            }
          }

          if (!UserVectorized) {
            NeedExtract = true;
            break;
          }
        }
      }

      if (NeedExtract)
        Obj += LaneCosts[Lane];
    }
  }

  return Obj;
}

static bool conflictsWithChosen(const CandidatePairs &C,
                                const std::vector<bool> &Chosen,
                                uint32_t PackIdx) {
  if (PackIdx >= C.CircularConflicts.size())
    return false;
  for (uint32_t Other : C.CircularConflicts[PackIdx]) {
    if (Other < Chosen.size() && Chosen[Other])
      return true;
  }
  return false;
}

} // namespace

std::vector<bool> solveILP(const CandidatePairs &C, const ILPModel &Model,
                           double TimeLimitSeconds) {
  const int N = static_cast<int>(C.Packs.size());
  std::vector<bool> Best(N, false);
  std::vector<bool> Cur(N, false);

  if (N == 0)
    return Best;

  std::vector<double> VecSavings(N, 0.0);
  for (int I = 0; I < N && I < static_cast<int>(Model.VecSavings.size()); ++I)
    VecSavings[I] = Model.VecSavings[I];

  std::vector<int> Order(N);
  std::iota(Order.begin(), Order.end(), 0);
  std::stable_sort(Order.begin(), Order.end(), [&](int A, int B) {
    return VecSavings[A] < VecSavings[B];
  });

  std::vector<double> SuffixNeg(N + 1, 0.0);
  for (int Pos = N - 1; Pos >= 0; --Pos) {
    double V = VecSavings[Order[Pos]];
    SuffixNeg[Pos] = SuffixNeg[Pos + 1] + (V < 0.0 ? V : 0.0);
  }

  std::unordered_set<const Instruction *> UsedInsts;

  // Greedy seed.
  for (int Pos = 0; Pos < N; ++Pos) {
    int Idx = Order[Pos];
    if (VecSavings[Idx] >= 0.0)
      continue;

    if (conflictsWithChosen(C, Cur, static_cast<uint32_t>(Idx)))
      continue;

    bool Overlap = false;
    for (const Instruction *I : C.Packs[Idx]) {
      if (UsedInsts.count(I)) {
        Overlap = true;
        break;
      }
    }
    if (Overlap)
      continue;

    Cur[Idx] = true;
    for (const Instruction *I : C.Packs[Idx])
      UsedInsts.insert(I);
  }

  double BestObjective = evaluateObjective(C, Model, Cur);
  Best = Cur;

  // Reset state for DFS.
  std::fill(Cur.begin(), Cur.end(), false);
  UsedInsts.clear();

  auto Start = std::chrono::steady_clock::now();
  auto Deadline = Start + std::chrono::duration<double>(TimeLimitSeconds);
  bool TimeLimitHit = false;

  std::function<void(int, double)> DFS = [&](int Pos, double LinearCost) {
    if (TimeLimitHit)
      return;

    if (TimeLimitSeconds > 0.0 && std::chrono::steady_clock::now() > Deadline) {
      TimeLimitHit = true;
      return;
    }

    if (Pos == N) {
      double Obj = evaluateObjective(C, Model, Cur);
      if (Obj < BestObjective) {
        BestObjective = Obj;
        Best = Cur;
      }
      return;
    }

    // Lower bound: only linear VS terms from undecided variables.
    double LB = LinearCost + SuffixNeg[Pos];
    if (LB > BestObjective)
      return;

    int Idx = Order[Pos];

    // Branch 1: skip.
    Cur[Idx] = false;
    DFS(Pos + 1, LinearCost);

    // Branch 2: take.
    if (conflictsWithChosen(C, Cur, static_cast<uint32_t>(Idx)))
      return;

    bool Overlap = false;
    for (const Instruction *I : C.Packs[Idx]) {
      if (UsedInsts.count(I)) {
        Overlap = true;
        break;
      }
    }
    if (Overlap)
      return;

    Cur[Idx] = true;
    std::vector<const Instruction *> Inserted;
    Inserted.reserve(C.Packs[Idx].size());
    for (const Instruction *I : C.Packs[Idx]) {
      auto Res = UsedInsts.insert(I);
      if (Res.second)
        Inserted.push_back(I);
    }

    DFS(Pos + 1, LinearCost + VecSavings[Idx]);

    for (const Instruction *I : Inserted)
      UsedInsts.erase(I);
    Cur[Idx] = false;
  };

  DFS(0, 0.0);
  return Best;
}
