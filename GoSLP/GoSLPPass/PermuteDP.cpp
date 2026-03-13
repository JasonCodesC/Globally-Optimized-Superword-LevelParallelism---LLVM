#include "PermuteDP.hpp"

namespace {

static bool isIdentity(const Permutation &P) {
  for (unsigned I = 0; I < P.size(); ++I) {
    if (P[I] != I)
      return false;
  }
  return true;
}

static bool containsPerm(const PermsList &L, const Permutation &P) {
  for (const auto &X : L) {
    if (X == P)
      return true;
  }
  return false;
}

static void appendUnique(PermsList &Dst, const Permutation &P) {
  if (!containsPerm(Dst, P))
    Dst.push_back(P);
}

static std::vector<const Instruction *>
applyPermutation(const std::vector<const Instruction *> &Pack,
                 const Permutation &P) {
  std::vector<const Instruction *> Out(Pack.size());
  for (size_t I = 0; I < Pack.size(); ++I)
    Out[I] = Pack[P[I]];
  return Out;
}

static bool isConstrainedNode(const Node &N) {
  if (N.pack.empty())
    return true;

  bool AllLoads = true;
  bool AllStores = true;
  for (const Instruction *I : N.pack) {
    AllLoads &= isa<LoadInst>(I);
    AllStores &= isa<StoreInst>(I);
  }
  return AllLoads || AllStores;
}

} // namespace

PermsList generatePerms(unsigned Width) {
  PermsList List;
  if (Width == 0)
    return List;

  Permutation Base(Width);
  for (unsigned I = 0; I < Width; ++I)
    Base[I] = I;

  if (Width <= 4) {
    Permutation P = Base;
    List.push_back(P);
    while (std::next_permutation(P.begin(), P.end()))
      List.push_back(P);
  } else {
    List.push_back(Base);
  }

  return List;
}

Perms choosePermutationsDP(const VecGraph &G, const std::vector<bool> &Chosen,
                           ShuffleCost &SC) {
  Perms Result;
  const unsigned N = G.items.size();
  if (N == 0 || Chosen.size() < N)
    return Result;

  std::vector<int> InDeg(N, 0);
  for (unsigned I = 0; I < N; ++I) {
    if (!Chosen[I])
      continue;
    for (int U : G.items[I].Uses) {
      if (U >= 0 && static_cast<unsigned>(U) < N && Chosen[U])
        ++InDeg[U];
    }
  }

  std::queue<int> Q;
  for (unsigned I = 0; I < N; ++I) {
    if (Chosen[I] && InDeg[I] == 0)
      Q.push(static_cast<int>(I));
  }

  std::vector<int> Topo;
  while (!Q.empty()) {
    int V = Q.front();
    Q.pop();
    Topo.push_back(V);

    for (int U : G.items[V].Uses) {
      if (U < 0 || static_cast<unsigned>(U) >= N || !Chosen[U])
        continue;
      if (--InDeg[U] == 0)
        Q.push(U);
    }
  }

  // Candidate permutation sets.
  std::vector<PermsList> Candidates(N);
  for (unsigned I = 0; I < N; ++I) {
    if (!Chosen[I])
      continue;

    Permutation Identity(G.items[I].pack.size());
    for (unsigned L = 0; L < Identity.size(); ++L)
      Identity[L] = L;

    if (isConstrainedNode(G.items[I])) {
      Candidates[I].push_back(Identity);
    } else {
      Candidates[I] = generatePerms(G.items[I].pack.size());
      if (Candidates[I].empty())
        Candidates[I].push_back(Identity);
    }
  }

  // Forward/backward propagation of profitable candidate masks.
  std::vector<PermsList> Forward(N), Backward(N);

  for (int V : Topo) {
    if (!Chosen[V])
      continue;

    if (isConstrainedNode(G.items[V])) {
      Forward[V] = Candidates[V];
      continue;
    }

    for (int D : G.items[V].Defs) {
      if (D < 0 || static_cast<unsigned>(D) >= N || !Chosen[D])
        continue;
      for (const auto &P : Forward[D])
        appendUnique(Forward[V], P);
    }

    if (Forward[V].empty())
      Forward[V] = Candidates[V];
  }

  for (auto It = Topo.rbegin(); It != Topo.rend(); ++It) {
    int V = *It;
    if (!Chosen[V])
      continue;

    if (isConstrainedNode(G.items[V])) {
      Backward[V] = Candidates[V];
      continue;
    }

    for (int U : G.items[V].Uses) {
      if (U < 0 || static_cast<unsigned>(U) >= N || !Chosen[U])
        continue;
      for (const auto &P : Backward[U])
        appendUnique(Backward[V], P);
    }

    if (Backward[V].empty())
      Backward[V] = Candidates[V];
  }

  for (unsigned I = 0; I < N; ++I) {
    if (!Chosen[I])
      continue;

    if (isConstrainedNode(G.items[I]))
      continue;

    PermsList Filtered;
    for (const auto &P : Forward[I])
      appendUnique(Filtered, P);
    for (const auto &P : Backward[I])
      appendUnique(Filtered, P);

    if (!Filtered.empty())
      Candidates[I] = std::move(Filtered);
  }

  std::vector<CostVec> DP(N);
  for (unsigned I = 0; I < N; ++I) {
    if (!Chosen[I])
      continue;
    DP[I] = CostVec(Candidates[I].size(), InstructionCost(0));
  }

  for (auto It = Topo.rbegin(); It != Topo.rend(); ++It) {
    int V = *It;
    if (!Chosen[V])
      continue;

    const auto &PermsV = Candidates[V];
    if (PermsV.empty())
      continue;

    for (size_t PI = 0; PI < PermsV.size(); ++PI) {
      Node VPerm = G.items[V];
      VPerm.pack = applyPermutation(G.items[V].pack, PermsV[PI]);

      InstructionCost Total = 0;
      for (int S : G.items[V].Uses) {
        if (S < 0 || static_cast<unsigned>(S) >= N || !Chosen[S])
          continue;

        const auto &PermsS = Candidates[S];
        InstructionCost Best = InstructionCost::getInvalid();
        for (size_t SJ = 0; SJ < PermsS.size(); ++SJ) {
          Node SPerm = G.items[S];
          SPerm.pack = applyPermutation(G.items[S].pack, PermsS[SJ]);
          InstructionCost Cand = SC.getShuffleCost(VPerm, SPerm) + DP[S][SJ];
          if (!Best.isValid() || Cand < Best)
            Best = Cand;
        }

        if (Best.isValid())
          Total += Best;
      }

      DP[V][PI] = Total;
    }
  }

  for (int V : Topo) {
    if (!Chosen[V] || Candidates[V].empty())
      continue;

    size_t BestIdx = 0;
    for (size_t I = 1; I < Candidates[V].size(); ++I) {
      if (DP[V][I] < DP[V][BestIdx])
        BestIdx = I;
    }

    // Preserve strict order for constrained memory nodes.
    if (isConstrainedNode(G.items[V])) {
      const auto &P = Candidates[V][BestIdx];
      if (!isIdentity(P)) {
        Result[V] = Permutation(P.size());
        for (unsigned L = 0; L < P.size(); ++L)
          Result[V][L] = L;
        continue;
      }
    }

    Result[V] = Candidates[V][BestIdx];
  }

  return Result;
}
