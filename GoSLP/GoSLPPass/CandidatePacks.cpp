/*

CanditdatePacks.cpp

This file collects legal candidate packs via the methods and constraints specified in the GoSLP paper

*/
#include "CandidatePacks.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/SimplifyQuery.h"
#include "llvm/Analysis/WithCache.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/FMF.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Compiler.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/DenseMap.h"
using namespace llvm;

// return true if load/store
bool accessesMemory(const Instruction *I) {
  return isa<LoadInst>(I) || isa<StoreInst>(I);
}

// returns true if fp, int, or vector type
bool isScalarOrVectorIntOrFP(Type *Ty) {
  if (Ty->isIntegerTy() || Ty->isFloatingPointTy()) {
    return true;
  }

  if (auto *VTy = dyn_cast<VectorType>(Ty)) {
    Type *EltTy = VTy->getElementType();
    return EltTy->isIntegerTy() || EltTy->isFloatingPointTy();
  }
  return false;
}

// Isomorphic like in 3.1 from the paper
bool areIsomorphic(const Instruction *I1, const Instruction *I2) {
  // loads
  if (auto *L1 = dyn_cast<LoadInst>(I1)) {
    auto *L2 = dyn_cast<LoadInst>(I2);
    if (!L2) {
      return false;
    }
    if (L1->getType() != L2->getType()) {
      return false;
    }
    if (L1->getPointerAddressSpace() != L2->getPointerAddressSpace()) {
      return false;
    }
    return true;
  }
  // stores
  if (auto *S1 = dyn_cast<StoreInst>(I1)) {
    auto *S2 = dyn_cast<StoreInst>(I2);
    if (!S2) {
      return false;
    }
    if (S1->getValueOperand()->getType() != S2->getValueOperand()->getType()) {
      return false;
    }
    if (S1->getPointerAddressSpace() != S2->getPointerAddressSpace()) {
      return false;
    }
    return true;
  }
  // same ALU ops
  if (auto *BO1 = dyn_cast<BinaryOperator>(I1)) {
    auto *BO2 = dyn_cast<BinaryOperator>(I2);
    if (!BO2) {
      return false;
    }
    // same type
    Type *Ty1 = BO1->getType();
    Type *Ty2 = BO2->getType();
    if (Ty1 != Ty2) {
      return false;
    }
    // ensure int, fp, or vector
    if (!isScalarOrVectorIntOrFP(Ty1)) {
      return false;
    }
    // same instru
    if (BO1->getOpcode() != BO2->getOpcode()) {
      return false;
    }
    // same operand count and operand types.
    if (BO1->getNumOperands() != BO2->getNumOperands()) {
      return false;
    }
    for (uint32_t k = 0; k < BO1->getNumOperands(); ++k) {
      if (BO1->getOperand(k)->getType() != BO2->getOperand(k)->getType()) {
        return false;
      }
    }
    return true;
  }
  // fmuladd intrinsic calls
  if (const auto *CI1 = dyn_cast<CallInst>(I1)) {
    const auto *CI2 = dyn_cast<CallInst>(I2);
    if (!CI2)
      return false;
    auto *F1 = CI1->getCalledFunction();
    auto *F2 = CI2->getCalledFunction();
    if (!F1 || !F2)
      return false;
    if (F1->getIntrinsicID() != Intrinsic::fmuladd ||
        F2->getIntrinsicID() != Intrinsic::fmuladd)
      return false;
    if (CI1->getType() != CI2->getType())
      return false;
    if (CI1->arg_size() != CI2->arg_size())
      return false;
    for (unsigned i = 0; i < CI1->arg_size(); ++i) {
      if (CI1->getArgOperand(i)->getType() != CI2->getArgOperand(i)->getType())
        return false;
    }
    return true;
  }
  // not load, store, alu or int, fp, vector
  return false;
}

// compute base and offset for a load/store
// bool getAddrBaseAndOffset(const Instruction *I, const DataLayout &DL,
//         const Value *&Base, int64_t &ByteOffset) {

//   Value *Ptr = nullptr;
//   Type *ElemTy = nullptr;

//   if (auto const *L = dyn_cast<LoadInst>(I)) {
//     Ptr = const_cast<Value *>(L->getPointerOperand());
//     ElemTy = L->getType();
//   } 
//   else if (auto const *S = dyn_cast<StoreInst>(I)) {
//     Ptr = const_cast<Value *>(S->getPointerOperand());
//     ElemTy = S->getValueOperand()->getType();
//   } 
//   else {
//     return false;
//   }

//   if (!ElemTy->isSized())
//     return false;

//   APInt Offset(DL.getPointerSizeInBits(0), 0);

//   Ptr = Ptr->stripAndAccumulateInBoundsConstantOffsets(DL, Offset);
//   Base = Ptr->stripInBoundsConstantOffsets();
//   ByteOffset = Offset.getSExtValue();

//   return true;
// }

// compute base and offset for a load/store
bool getAddrBaseAndOffset(const Instruction *I, const DataLayout &DL,
                          const Value *&Base, int64_t &ByteOffset) {
  const Value *Ptr = nullptr;
  Type *ElemTy = nullptr;

  if (auto const *L = dyn_cast<LoadInst>(I)) {
    Ptr = L->getPointerOperand();                 // e.g. %12 or %19
    ElemTy = L->getType();                        // i32
  } else if (auto const *S = dyn_cast<StoreInst>(I)) {
    Ptr = S->getPointerOperand();
    ElemTy = S->getValueOperand()->getType();
  } else {
    return false;
  }

  if (!ElemTy->isSized())
    return false;

  // Start from the pointer operand and strip inbounds constant GEPs,
  // accumulating the byte offset.
  APInt Offset(DL.getIndexTypeSizeInBits(Ptr->getType()), 0);
  const Value *PtrNoConstGEP =
      Ptr->stripAndAccumulateInBoundsConstantOffsets(DL, Offset);

  // Canonical base: underlying object (alloca, global, arg, etc.).
  // This makes both a[0] and a[1] see the same Base.
  // Base = getUnderlyingObject(PtrNoConstGEP, DL);
  Base = getUnderlyingObject(PtrNoConstGEP);

  ByteOffset = Offset.getSExtValue();
  return true;
}

bool areLoadsEquivalent(const Instruction *A, const Instruction *B) {
    if (!A || !B) return false;
    if (A->getOpcode() != B->getOpcode())
        return false;

    // Must be load
    if (A->getOpcode() != Instruction::Load)
        return false;

    const LoadInst *LA = cast<LoadInst>(A);
    const LoadInst *LB = cast<LoadInst>(B);

    if (LA->getType() != LB->getType()) return false;
    if (LA->getAlign() != LB->getAlign()) return false;
    if (LA->isVolatile() != LB->isVolatile()) return false;

    return true;
}

// same base object and offset differs by exactly one
bool areAdjacentMemoryAccesses(const Instruction *I1, const Instruction *I2,
  const DataLayout &DL, AAResults &AA) {

  if (!accessesMemory(I1) || !accessesMemory(I2)) {
    return false;
  }
  // only consider load-load or store-store
  bool I1IsLoad = isa<LoadInst>(I1);
  bool I2IsLoad = isa<LoadInst>(I2);
  if (I1IsLoad != I2IsLoad) {
    return false;
  }
  // get base + offset
  const Value *Base1 = nullptr, *Base2 = nullptr;
  int64_t Off1 = 0, Off2 = 0;
  if (!getAddrBaseAndOffset(I1, DL, Base1, Off1)) {
    return false;
  }
  if (!getAddrBaseAndOffset(I2, DL, Base2, Off2)) {
    return false;
  }
  if (!areLoadsEquivalent(I1, I2)) {
    return false;
  }
  // If AA can prove they do not alias, bail; otherwise allow possibly-aliasing
  // bases (this is important in unrolled loops where pointers compare
  // differently but still walk the same underlying array).
  if (AA.isNoAlias(MemoryLocation::get(I1), MemoryLocation::get(I2)))
    return false;

  // ensure elements are adjacent
  Type *ElemTy = nullptr;
  if (auto *L = dyn_cast<LoadInst>(I1)) {
    ElemTy = L->getType();
  }
  else {
    auto *S = cast<StoreInst>(I1);
    ElemTy = S->getValueOperand()->getType();
  }
  if (!ElemTy->isSized()) {
    return false;
  }
  uint64_t ElemSize = DL.getTypeStoreSize(ElemTy);
  int64_t Diff = Off1 - Off2;
  if (Diff < 0) {
    Diff = -Diff;
  }
  // return true if they are adjacent
  return Diff == static_cast<int64_t>(ElemSize);
}

// bfs to ensure the 2 instructions arent dependent so we can pack them
bool isTransitivelyDependent(Instruction *From, Instruction *To, MemorySSA &MSSA) {

  if (From == To) {
    return true;
  }

  std::unordered_set<Instruction *> Visited;
  std::queue<Instruction *> Q;
  Visited.insert(From);
  Q.push(From);

  while (!Q.empty()) {
    Instruction *Cur = Q.front();
    Q.pop();

    // dependence so bad
    if (Cur == To)
      return true;

    // push users to q (SSA def-use)
    for (User *U : Cur->users()) {
      if (auto *UI = dyn_cast<Instruction>(U)) {
        if (Visited.insert(UI).second) {
          Q.push(UI);
        }
      }
    }

    // If this instruction writes to memory, follow the MemorySSA def-use edges
    // that it actually clobbers. This captures store->load/store dependences
    // without treating unrelated reads as dependent.
    if (Cur->mayWriteToMemory()) {
      if (MemoryAccess *MA = MSSA.getMemoryAccess(Cur)) {
        if (MemorySSAWalker *W = MSSA.getWalker()) {
          for (User *MU : MA->users()) {
            if (auto *MUOD = dyn_cast<MemoryUseOrDef>(MU)) {
              if (W->getClobberingMemoryAccess(MUOD) != MA)
                continue;
              Instruction *MemI = MUOD->getMemoryInst();
              if (Visited.insert(MemI).second) {
                Q.push(MemI);
              }
            }
          }
        }
      }
    }
  }

  return false;
}

// ensure no dependence either way.
bool areIndependent(Instruction *I1, Instruction *I2, MemorySSA &MSSA) {
  if (isTransitivelyDependent(I1, I2, MSSA) ||
      isTransitivelyDependent(I2, I1, MSSA)) {
    return false;
  }

  // If neither writes to memory, SSA def-use is the only dependency.
  if (!I1->mayWriteToMemory() && !I2->mayWriteToMemory())
    return true;

  MemoryAccess *MA1 = MSSA.getMemoryAccess(I1);
  MemoryAccess *MA2 = MSSA.getMemoryAccess(I2);
  if (!MA1 || !MA2)
    return true;

  if (MemorySSAWalker *W = MSSA.getWalker()) {
    if (W->getClobberingMemoryAccess(MA2) == MA1)
      return false;
    if (W->getClobberingMemoryAccess(MA1) == MA2)
      return false;
  }

  return true;
}

// Find the next instruction in the same block that is a candidate statement.
static const Instruction *nextCandidate(const Instruction *I) {
  const Instruction *Cur = I;
  while ((Cur = Cur->getNextNonDebugInstruction())) {
    if (isCandidateStatement(const_cast<Instruction *>(Cur)))
      return Cur;
  }
  return nullptr;
}

// goSLP states that instructions must be in the same BB to be paired
bool areSchedulableTogether(Instruction *I1, Instruction *I2) {
  return I1->getParent() == I2->getParent();
}

// add a 2-wide pack
void addPack(CandidatePairs &C, const Instruction *I1, const Instruction *I2) {

  uint32_t Index = C.Packs.size();
  std::vector<const Instruction *> Pack;
  Pack.push_back(I1);
  Pack.push_back(I2);
  C.Packs.push_back(std::move(Pack));

  CandidateId Id;
  Id.Width = 2;
  Id.Index = Index;
  C.InstToCandidates[I1].push_back(Id);
  C.InstToCandidates[I2].push_back(Id);
}

// ensure candidate statements, load, store, alu
bool isCandidateStatement(Instruction *I) {
  if (auto *L = dyn_cast<LoadInst>(I)) {
    Type *Ty = L->getType();
    return isScalarOrVectorIntOrFP(Ty);
  }

  if (auto *S = dyn_cast<StoreInst>(I)) {
    Type *ValTy = S->getValueOperand()->getType();
    return isScalarOrVectorIntOrFP(ValTy);
  }

  if (auto *BO = dyn_cast<BinaryOperator>(I)) {
    Type *Ty = BO->getType();
    return isScalarOrVectorIntOrFP(Ty);
  }

  if (auto *CI = dyn_cast<CallInst>(I)) {
    if (Function *F = CI->getCalledFunction()) {
      if (F->getIntrinsicID() == Intrinsic::fmuladd)
        return true;
    }
  }

  return false;
}

// Check all goSLP §3.1 constraints for a *pair* (Si, Sj) in the same BB.
bool legalGoSLPPair(Instruction *I1, Instruction *I2, const DataLayout &DL,
    AAResults &AA, MemorySSA &MSSA) {

  if (I1 == I2) {
    return false;
  }
  if (!areIsomorphic(I1, I2)) {
    return false;
  }
  if (!areIndependent(I1, I2, MSSA)) {
    return false;
  }
  if (!areSchedulableTogether(I1, I2)) {
    return false;
  }
  if (accessesMemory(I1) || accessesMemory(I2)) {
    if (!areAdjacentMemoryAccesses(I1, I2, DL, AA)) {
      return false;
    }
  }

  return true;
}


// main function
static void widenPacks(CandidatePairs &C, const DataLayout &DL, AAResults &AA,
                       unsigned MaxWidth = 32);

CandidatePairs collectCandidatePairs(Function &F, AAResults &AA, MemorySSA &MSSA) {

  CandidatePairs Result;
  Module *M = F.getParent();
  if (!M) {
    return Result;
  }
  const DataLayout &DL = M->getDataLayout();

  // Whole-function vectorization unit (for later cost modeling), but pairs
  // are only formed inside each basic block.
  std::unordered_map<const Instruction *, uint32_t> Index;
  uint32_t Pos = 0;

  // assign indices to all candidate statements in function order.
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (!isCandidateStatement(&I)) {
        continue;
      }
      Index[&I] = Pos++;
    }
  }

  if (Index.empty())
    return Result;

  // for each basic block, form all legal pairs in that block.
  for (BasicBlock &BB : F) {
    std::vector<Instruction *> StmtsInBB;
    for (Instruction &I : BB) {
      if (!isCandidateStatement(&I)) {
        continue;
      }
      StmtsInBB.push_back(&I);
    }
    const uint32_t N = StmtsInBB.size();
    if (N < 2) {
      continue;
    }
    const uint32_t MaxNeighborDistance = 1; // only immediate neighbors to curb explosion
    for (uint32_t i = 0; i < N; ++i) {
      Instruction *I1 = StmtsInBB[i];
      uint32_t jEnd = std::min<uint32_t>(N, i + 1 + MaxNeighborDistance);
      for (uint32_t j = i + 1; j < jEnd; ++j) {
        Instruction *I2 = StmtsInBB[j];
        if (!areIsomorphic(I1, I2))
          continue;
        if (!areIndependent(I1, I2, MSSA))
          continue;
        if (!areSchedulableTogether(I1, I2))
          continue;
        if ((accessesMemory(I1) || accessesMemory(I2)) &&
            !areAdjacentMemoryAccesses(I1, I2, DL, AA))
          continue;

        addPack(Result, I1, I2);
      }
    }
  }

  // Pair by opcode (captures mul/add separated by other instructions) in each BB.
  for (BasicBlock &BB : F) {
    DenseMap<unsigned, std::vector<Instruction *>> OpsToInsts;
    std::vector<Instruction *> FMAInsts;
    for (Instruction &I : BB) {
      if (!isCandidateStatement(&I))
        continue;
      if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
        OpsToInsts[BO->getOpcode()].push_back(&I);
      } else if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (auto *F = CI->getCalledFunction()) {
          if (F->getIntrinsicID() == Intrinsic::fmuladd)
            FMAInsts.push_back(&I);
        }
      }
    }

    auto pairConsecutive = [&](std::vector<Instruction *> &Vec) {
      if (Vec.size() < 2)
        return;
      for (size_t i = 0; i + 1 < Vec.size(); ++i) {
        Instruction *A = Vec[i];
        Instruction *B = Vec[i + 1];
        if (legalGoSLPPair(A, B, DL, AA, MSSA))
          addPack(Result, A, B);
      }
    };

    for (auto &KV : OpsToInsts)
      pairConsecutive(KV.second);
    pairConsecutive(FMAInsts);
  }

  // Additional pass: build contiguous load/store packs per base/offset to
  // catch unrolled loop patterns.
  for (BasicBlock &BB : F) {
    struct MemAccessInfo {
      Instruction *I;
      const Value *Base;
      int64_t Offset;
      uint64_t ElemSize;
    };
    std::vector<MemAccessInfo> Loads;
    std::vector<MemAccessInfo> Stores;

    for (Instruction &I : BB) {
      const Value *Base = nullptr;
      int64_t Off = 0;
      if (!getAddrBaseAndOffset(&I, DL, Base, Off))
        continue;
      uint64_t ElemSize = 0;
      if (auto *L = dyn_cast<LoadInst>(&I))
        ElemSize = DL.getTypeStoreSize(L->getType());
      else if (auto *S = dyn_cast<StoreInst>(&I))
        ElemSize = DL.getTypeStoreSize(S->getValueOperand()->getType());
      else
        continue;
      if (ElemSize == 0)
        continue;
      MemAccessInfo Info{&I, Base, Off, ElemSize};
      if (isa<LoadInst>(&I))
        Loads.push_back(Info);
      else
        Stores.push_back(Info);
    }

    auto buildContiguousPairs = [&](std::vector<MemAccessInfo> &Vec) {
      llvm::stable_sort(Vec, [](const MemAccessInfo &A, const MemAccessInfo &B) {
        if (A.Base != B.Base)
          return A.Base < B.Base;
        return A.Offset < B.Offset;
      });
      for (size_t idx = 1; idx < Vec.size(); ++idx) {
        auto &Prev = Vec[idx - 1];
        auto &Cur = Vec[idx];
        if (Prev.Base != Cur.Base)
          continue;
        if (Prev.ElemSize != Cur.ElemSize)
          continue;
        if (Cur.Offset - Prev.Offset != static_cast<int64_t>(Prev.ElemSize))
          continue;
        addPack(Result, Prev.I, Cur.I);
      }
    };

    buildContiguousPairs(Loads);
    buildContiguousPairs(Stores);
  }

  // Clamp pack count to keep solver/emitter manageable.
  const size_t PackLimit = 64;
  if (Result.Packs.size() > PackLimit) {
    auto Packs = Result.Packs;
    Result.Packs.assign(Packs.begin(), Packs.begin() + PackLimit);
    Result.InstToCandidates.clear();
    for (size_t idx = 0; idx < Result.Packs.size(); ++idx) {
      CandidateId Id{static_cast<uint32_t>(Result.Packs[idx].size()),
                     static_cast<uint32_t>(idx)};
      for (const Instruction *Inst : Result.Packs[idx]) {
        Result.InstToCandidates[Inst].push_back(Id);
      }
    }
  }

  // Iterative widening over the collected packs.
  if (Result.Packs.size() <= 256)
    widenPacks(Result, DL, AA, /*MaxWidth=*/32);

  return Result;
}


// Iteratively widen packs: start from existing packs and merge same-width
// isomorphic packs in the same block until no change or we hit MaxWidth.
static void widenPacks(CandidatePairs &C, const DataLayout &DL, AAResults &AA,
                       unsigned MaxWidth) {
  bool changed = true;
  const size_t MaxPackCount = 2000;
  while (changed) {
    changed = false;
    std::vector<std::vector<const Instruction *>> NewPacks;
    size_t mergeBudget = 128; // avoid blow-up

    if (C.Packs.size() > MaxPackCount)
      break;

    for (size_t i = 0; i < C.Packs.size(); ++i) {
      const auto &P1 = C.Packs[i];
      if (P1.empty())
        continue;
      unsigned W = P1.size();
      if (W >= MaxWidth)
        continue;
      const BasicBlock *BB = P1[0]->getParent();
      if (!BB)
        continue;
      // Only widen packs whose instructions are all isomorphic to peers; we
      // rely on areIsomorphic per-lane below.

      for (size_t j = i + 1; j < C.Packs.size(); ++j) {
        const auto &P2 = C.Packs[j];
        if (P2.size() != W)
          continue;
        if (P2[0]->getParent() != BB)
          continue;
        if (mergeBudget == 0)
          break;

        bool same = true;
        for (size_t k = 0; k < P1.size(); ++k) {
          if (!areIsomorphic(P1[k], P2[k]) || P1[k] == P2[k]) {
            same = false;
            break;
          }
        }
        if (!same)
          continue;

        bool overlap = false;
        for (const Instruction *A : P1) {
          if (llvm::is_contained(P2, A)) {
            overlap = true;
            break;
          }
        }
        if (overlap)
          continue;

        // Keep widening deterministic: require P1 precedes P2 within a small
        // candidate gap window inside the same block.
        const unsigned MaxCandidateGap = 3;
        unsigned GapCount = 0;
        const Instruction *Cur = P1.back();
        while ((Cur = nextCandidate(Cur))) {
          ++GapCount;
          if (Cur == P2.front())
            break;
          if (GapCount > MaxCandidateGap)
            break;
        }
        if (Cur != P2.front())
          continue;

        // For memory instructions, enforce a contiguous base/offset chain so we
        // do not merge unrelated accesses.
        if (accessesMemory(P1.back()) || accessesMemory(P2.front())) {
          if (!accessesMemory(P1.back()) || !accessesMemory(P2.front()))
            continue;
          if (!areIsomorphic(P1.front(), P2.front()))
            continue;
          if (!areAdjacentMemoryAccesses(P1.back(), P2.front(), DL, AA))
            continue;
        }

        std::vector<const Instruction *> widened;
        widened.reserve(P1.size() + P2.size());
        widened.insert(widened.end(), P1.begin(), P1.end());
        widened.insert(widened.end(), P2.begin(), P2.end());
        NewPacks.push_back(std::move(widened));
        if (--mergeBudget == 0)
          break;
      }
      if (mergeBudget == 0)
        break;
    }

    if (!NewPacks.empty()) {
      changed = true;
      for (auto &NP : NewPacks) {
        uint32_t Index = C.Packs.size();
        C.Packs.push_back(NP);
        CandidateId Id{static_cast<uint32_t>(NP.size()), Index};
        for (const Instruction *Inst : NP) {
          C.InstToCandidates[Inst].push_back(Id);
        }
      }
    }
  }

  // Final clamp to keep solver manageable after widening.
  const size_t TotalLimit = 128;
  if (C.Packs.size() > TotalLimit) {
    auto Packs = C.Packs;
    C.Packs.assign(Packs.begin(), Packs.begin() + TotalLimit);
    C.InstToCandidates.clear();
    for (size_t idx = 0; idx < C.Packs.size(); ++idx) {
      CandidateId Id{static_cast<uint32_t>(C.Packs[idx].size()),
                     static_cast<uint32_t>(idx)};
      for (const Instruction *Inst : C.Packs[idx]) {
        C.InstToCandidates[Inst].push_back(Id);
      }
    }
  }
}


void printCandidatePairs(const CandidatePairs &CP) {
    errs() << "===== CandidatePairs =====\n";

    // ---- Print Packs (truncated) ----
    const size_t Limit = 80;
    errs() << "Packs (" << CP.Packs.size() << "): showing first "
           << std::min(Limit, CP.Packs.size()) << "\n";
    for (size_t i = 0; i < CP.Packs.size() && i < Limit; ++i) {
        errs() << "  Pack " << i << ":\n";
        for (const Instruction *Inst : CP.Packs[i]) {
            errs() << "    ";
            if (Inst)
                Inst->print(errs());
            else
                errs() << "<null inst>";
            errs() << "\n";
        }
    }
    if (CP.Packs.size() > Limit)
      errs() << "  ... (" << (CP.Packs.size() - Limit) << " more packs elided)\n";

    // ---- Print InstToCandidates (summary) ----
    errs() << "InstToCandidates (" << CP.InstToCandidates.size() << ")\n";
    errs() << "==========================\n";
}
