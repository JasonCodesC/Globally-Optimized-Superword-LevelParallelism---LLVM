#include "CandidatePacks.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

static bool containsPack(const std::vector<uint32_t> &V, uint32_t Idx) {
  return llvm::is_contained(V, Idx);
}

static ValuePackKey canonicalizeLaneValues(ArrayRef<const Value *> Lanes) {
  ValuePackKey K;
  K.Lanes.assign(Lanes.begin(), Lanes.end());
  llvm::stable_sort(K.Lanes, [](const Value *A, const Value *B) {
    return std::less<const Value *>{}(A, B);
  });
  return K;
}

static ValuePackKey canonicalizeLaneInsts(
    const std::vector<const Instruction *> &Pack) {
  SmallVector<const Value *, 8> Vals;
  Vals.reserve(Pack.size());
  for (const Instruction *I : Pack)
    Vals.push_back(I);
  return canonicalizeLaneValues(Vals);
}

static void appendUnique(std::vector<uint32_t> &V, uint32_t Idx) {
  if (!containsPack(V, Idx))
    V.push_back(Idx);
}

static void addPackUnique(
    CandidatePairs &C,
    std::unordered_map<ValuePackKey, uint32_t, ValuePackKeyHash> &PackToIdx,
    const std::vector<const Instruction *> &Pack) {
  ValuePackKey K = canonicalizeLaneInsts(Pack);
  if (PackToIdx.find(K) != PackToIdx.end())
    return;

  uint32_t Idx = static_cast<uint32_t>(C.Packs.size());
  C.Packs.push_back(Pack);
  PackToIdx.emplace(std::move(K), Idx);
}

static void rebuildInstToCandidates(CandidatePairs &C) {
  C.InstToCandidates.clear();
  for (uint32_t Idx = 0; Idx < C.Packs.size(); ++Idx) {
    CandidateId Id{static_cast<uint32_t>(C.Packs[Idx].size()), Idx};
    for (const Instruction *I : C.Packs[Idx])
      C.InstToCandidates[I].push_back(Id);
  }
}

static bool packsOverlap(const std::vector<const Instruction *> &A,
                         const std::vector<const Instruction *> &B) {
  for (const Instruction *I : A) {
    if (llvm::is_contained(B, I))
      return true;
  }
  return false;
}

static bool canMergePacks(const std::vector<const Instruction *> &P1,
                          const std::vector<const Instruction *> &P2,
                          const DataLayout &DL, AAResults &AA,
                          MemorySSA &MSSA) {
  if (P1.size() != P2.size() || P1.empty())
    return false;

  if (packsOverlap(P1, P2))
    return false;

  for (unsigned Lane = 0; Lane < P1.size(); ++Lane) {
    Instruction *I1 = const_cast<Instruction *>(P1[Lane]);
    Instruction *I2 = const_cast<Instruction *>(P2[Lane]);
    if (!legalGoSLPPair(I1, I2, DL, AA, MSSA))
      return false;
  }

  return true;
}

static void widenPacks(CandidatePairs &C, const DataLayout &DL, AAResults &AA,
                       MemorySSA &MSSA, unsigned MaxWidth,
                       std::unordered_map<ValuePackKey, uint32_t,
                                          ValuePackKeyHash> &PackToIdx) {
  bool Changed = true;
  while (Changed) {
    Changed = false;
    std::vector<std::vector<const Instruction *>> NewPacks;

    for (size_t I = 0; I < C.Packs.size(); ++I) {
      const auto &P1 = C.Packs[I];
      if (P1.empty() || P1.size() >= MaxWidth)
        continue;

      for (size_t J = I + 1; J < C.Packs.size(); ++J) {
        const auto &P2 = C.Packs[J];
        if (P1.size() != P2.size())
          continue;

        if (!canMergePacks(P1, P2, DL, AA, MSSA))
          continue;

        std::vector<const Instruction *> Wider;
        Wider.reserve(P1.size() + P2.size());
        Wider.insert(Wider.end(), P1.begin(), P1.end());
        Wider.insert(Wider.end(), P2.begin(), P2.end());

        ValuePackKey K = canonicalizeLaneInsts(Wider);
        if (PackToIdx.find(K) != PackToIdx.end())
          continue;

        NewPacks.push_back(std::move(Wider));
        PackToIdx.emplace(std::move(K),
                          static_cast<uint32_t>(C.Packs.size() +
                                                NewPacks.size() - 1));
      }
    }

    if (!NewPacks.empty()) {
      Changed = true;
      for (auto &P : NewPacks)
        C.Packs.push_back(std::move(P));
    }
  }
}

static void buildUseMaps(CandidatePairs &C) {
  C.VecVecUses.clear();
  C.NonVecPacks.clear();
  C.NonVecPackToIndex.clear();
  C.NonVecVecUses.clear();

  C.LaneUses.assign(C.Packs.size(), {});
  for (size_t P = 0; P < C.Packs.size(); ++P)
    C.LaneUses[P].assign(C.Packs[P].size(), LaneUseInfo{});

  std::unordered_map<ValuePackKey, std::vector<uint32_t>, ValuePackKeyHash>
      CandidateByKey;
  CandidateByKey.reserve(C.Packs.size() * 2 + 1);
  for (uint32_t P = 0; P < C.Packs.size(); ++P) {
    CandidateByKey[canonicalizeLaneInsts(C.Packs[P])].push_back(P);
  }

  for (uint32_t P = 0; P < C.Packs.size(); ++P) {
    const auto &Pack = C.Packs[P];
    for (uint32_t Lane = 0; Lane < Pack.size(); ++Lane) {
      const Instruction *I = Pack[Lane];
      LaneUseInfo &Info = C.LaneUses[P][Lane];
      for (const User *U : I->users()) {
        auto *UI = dyn_cast<Instruction>(U);
        if (!UI) {
          Info.HasOutsideUse = true;
          continue;
        }
        (void)Info.UserToVectorUses[UI];
      }
    }
  }

  auto getOrCreateNonVec = [&](const ValuePackKey &Key) {
    auto It = C.NonVecPackToIndex.find(Key);
    if (It != C.NonVecPackToIndex.end())
      return It->second;

    uint32_t Idx = static_cast<uint32_t>(C.NonVecPacks.size());
    C.NonVecPacks.push_back(Key);
    C.NonVecPackToIndex.emplace(Key, Idx);
    return Idx;
  };

  for (uint32_t UsePackIdx = 0; UsePackIdx < C.Packs.size(); ++UsePackIdx) {
    const auto &UsePack = C.Packs[UsePackIdx];
    if (UsePack.empty())
      continue;

    SmallVector<unsigned, 4> OpIndices;
    if (isa<LoadInst>(UsePack.front())) {
      // Vector loads don't require explicit operand packing in IR.
      continue;
    } else if (isa<StoreInst>(UsePack.front())) {
      // Only the stored value contributes to vector operand packing.
      OpIndices.push_back(0);
    } else if (auto *CI = dyn_cast<CallInst>(UsePack.front())) {
      for (unsigned ArgIdx = 0; ArgIdx < CI->arg_size(); ++ArgIdx)
        OpIndices.push_back(ArgIdx);
    } else {
      for (unsigned OpIdx = 0; OpIdx < UsePack.front()->getNumOperands(); ++OpIdx)
        OpIndices.push_back(OpIdx);
    }

    for (unsigned OpIdx : OpIndices) {
      SmallVector<const Value *, 8> OperandLanes;
      OperandLanes.reserve(UsePack.size());
      bool AllInst = true;

      for (const Instruction *I : UsePack) {
        const Value *Op = nullptr;
        if (auto *CI = dyn_cast<CallInst>(I))
          Op = CI->getArgOperand(OpIdx);
        else
          Op = I->getOperand(OpIdx);
        OperandLanes.push_back(Op);
        if (!isa<Instruction>(Op))
          AllInst = false;
      }

      ValuePackKey OpKey = canonicalizeLaneValues(OperandLanes);
      auto CandIt = CandidateByKey.find(OpKey);

      if (AllInst && CandIt != CandidateByKey.end()) {
        for (uint32_t SrcPackIdx : CandIt->second) {
          if (SrcPackIdx == UsePackIdx)
            continue;

          appendUnique(C.VecVecUses[SrcPackIdx], UsePackIdx);

          const auto &SrcPack = C.Packs[SrcPackIdx];
          for (uint32_t UseLane = 0; UseLane < UsePack.size(); ++UseLane) {
            const Instruction *UseInst = UsePack[UseLane];
            const Value *OpVal = nullptr;
            if (auto *CI = dyn_cast<CallInst>(UseInst))
              OpVal = CI->getArgOperand(OpIdx);
            else
              OpVal = UseInst->getOperand(OpIdx);
            auto *OpInst = dyn_cast<Instruction>(OpVal);
            if (!OpInst)
              continue;

            for (uint32_t SrcLane = 0; SrcLane < SrcPack.size(); ++SrcLane) {
              if (SrcPack[SrcLane] != OpInst)
                continue;
              auto &LanesMap = C.LaneUses[SrcPackIdx][SrcLane].UserToVectorUses;
              const Instruction *UserInst = UseInst;
              appendUnique(LanesMap[UserInst], UsePackIdx);
            }
          }
        }
      } else {
        uint32_t NonVecIdx = getOrCreateNonVec(OpKey);
        appendUnique(C.NonVecVecUses[NonVecIdx], UsePackIdx);
      }
    }
  }
}

static void buildCircularConflicts(CandidatePairs &C, MemorySSA &MSSA) {
  const size_t ConflictBuildLimit = 96;
  C.CircularConflicts.assign(C.Packs.size(), {});
  if (C.Packs.size() > ConflictBuildLimit)
    return;

  for (uint32_t I = 0; I < C.Packs.size(); ++I) {
    for (uint32_t J = I + 1; J < C.Packs.size(); ++J) {
      if (packsOverlap(C.Packs[I], C.Packs[J]))
        continue;

      bool DepIJ = false;
      bool DepJI = false;
      for (const Instruction *A : C.Packs[I]) {
        for (const Instruction *B : C.Packs[J]) {
          DepIJ |= isTransitivelyDependent(const_cast<Instruction *>(A),
                                           const_cast<Instruction *>(B), MSSA);
          DepJI |= isTransitivelyDependent(const_cast<Instruction *>(B),
                                           const_cast<Instruction *>(A), MSSA);
          if (DepIJ && DepJI)
            break;
        }
        if (DepIJ && DepJI)
          break;
      }

      if (DepIJ && DepJI) {
        appendUnique(C.CircularConflicts[I], J);
        appendUnique(C.CircularConflicts[J], I);
      }
    }
  }
}

} // namespace

bool accessesMemory(const Instruction *I) {
  return isa<LoadInst>(I) || isa<StoreInst>(I);
}

bool isScalarOrVectorIntOrFP(Type *Ty) {
  if (Ty->isIntegerTy() || Ty->isFloatingPointTy())
    return true;

  if (auto *VTy = dyn_cast<VectorType>(Ty)) {
    Type *EltTy = VTy->getElementType();
    return EltTy->isIntegerTy() || EltTy->isFloatingPointTy();
  }

  return false;
}

bool areIsomorphic(const Instruction *I1, const Instruction *I2) {
  if (auto *L1 = dyn_cast<LoadInst>(I1)) {
    auto *L2 = dyn_cast<LoadInst>(I2);
    if (!L2)
      return false;
    if (L1->getType() != L2->getType())
      return false;
    if (L1->getPointerAddressSpace() != L2->getPointerAddressSpace())
      return false;
    return true;
  }

  if (auto *S1 = dyn_cast<StoreInst>(I1)) {
    auto *S2 = dyn_cast<StoreInst>(I2);
    if (!S2)
      return false;
    if (S1->getValueOperand()->getType() != S2->getValueOperand()->getType())
      return false;
    if (S1->getPointerAddressSpace() != S2->getPointerAddressSpace())
      return false;
    return true;
  }

  if (auto *BO1 = dyn_cast<BinaryOperator>(I1)) {
    auto *BO2 = dyn_cast<BinaryOperator>(I2);
    if (!BO2)
      return false;

    if (BO1->getOpcode() != BO2->getOpcode())
      return false;
    if (BO1->getType() != BO2->getType())
      return false;
    if (!isScalarOrVectorIntOrFP(BO1->getType()))
      return false;

    if (BO1->getNumOperands() != BO2->getNumOperands())
      return false;
    for (unsigned K = 0; K < BO1->getNumOperands(); ++K) {
      if (BO1->getOperand(K)->getType() != BO2->getOperand(K)->getType())
        return false;
    }
    return true;
  }

  if (auto *CI1 = dyn_cast<CallInst>(I1)) {
    auto *CI2 = dyn_cast<CallInst>(I2);
    if (!CI2)
      return false;
    Function *F1 = CI1->getCalledFunction();
    Function *F2 = CI2->getCalledFunction();
    if (!F1 || !F2)
      return false;
    if (F1->getIntrinsicID() != Intrinsic::fmuladd ||
        F2->getIntrinsicID() != Intrinsic::fmuladd)
      return false;

    if (CI1->getType() != CI2->getType())
      return false;
    if (CI1->arg_size() != CI2->arg_size())
      return false;
    for (unsigned I = 0; I < CI1->arg_size(); ++I) {
      if (CI1->getArgOperand(I)->getType() != CI2->getArgOperand(I)->getType())
        return false;
    }
    return true;
  }

  return false;
}

bool getAddrBaseAndOffset(const Instruction *I, const DataLayout &DL,
                          const Value *&Base, int64_t &ByteOffset) {
  const Value *Ptr = nullptr;
  Type *ElemTy = nullptr;

  if (auto *L = dyn_cast<LoadInst>(I)) {
    Ptr = L->getPointerOperand();
    ElemTy = L->getType();
  } else if (auto *S = dyn_cast<StoreInst>(I)) {
    Ptr = S->getPointerOperand();
    ElemTy = S->getValueOperand()->getType();
  } else {
    return false;
  }

  if (!ElemTy->isSized())
    return false;

  APInt Offset(DL.getIndexTypeSizeInBits(Ptr->getType()), 0);
  const Value *PtrNoConstGEP =
      Ptr->stripAndAccumulateInBoundsConstantOffsets(DL, Offset);
  Base = getUnderlyingObject(PtrNoConstGEP);
  ByteOffset = Offset.getSExtValue();
  return true;
}

bool areAdjacentMemoryAccesses(const Instruction *I1, const Instruction *I2,
                               const DataLayout &DL, AAResults &AA) {
  if (!accessesMemory(I1) || !accessesMemory(I2))
    return false;

  bool IsLoad1 = isa<LoadInst>(I1);
  bool IsLoad2 = isa<LoadInst>(I2);
  if (IsLoad1 != IsLoad2)
    return false;

  const Value *Base1 = nullptr;
  const Value *Base2 = nullptr;
  int64_t Off1 = 0;
  int64_t Off2 = 0;
  if (!getAddrBaseAndOffset(I1, DL, Base1, Off1) ||
      !getAddrBaseAndOffset(I2, DL, Base2, Off2))
    return false;

  if (Base1 != Base2)
    return false;

  Type *ElemTy = nullptr;
  if (auto *L = dyn_cast<LoadInst>(I1))
    ElemTy = L->getType();
  else
    ElemTy = cast<StoreInst>(I1)->getValueOperand()->getType();

  if (!ElemTy->isSized())
    return false;

  int64_t ElemSize = static_cast<int64_t>(DL.getTypeStoreSize(ElemTy));
  return std::abs(Off1 - Off2) == ElemSize;
}

bool isTransitivelyDependent(Instruction *From, Instruction *To, MemorySSA &MSSA) {
  if (From == To)
    return true;

  std::unordered_set<Instruction *> Visited;
  std::queue<Instruction *> Q;
  Visited.insert(From);
  Q.push(From);

  while (!Q.empty()) {
    Instruction *Cur = Q.front();
    Q.pop();

    if (Cur == To)
      return true;

    for (User *U : Cur->users()) {
      if (auto *UI = dyn_cast<Instruction>(U)) {
        if (Visited.insert(UI).second)
          Q.push(UI);
      }
    }

    if (Cur->mayWriteToMemory()) {
      if (MemoryAccess *MA = MSSA.getMemoryAccess(Cur)) {
        if (MemorySSAWalker *W = MSSA.getWalker()) {
          for (User *MU : MA->users()) {
            if (auto *MUOD = dyn_cast<MemoryUseOrDef>(MU)) {
              if (W->getClobberingMemoryAccess(MUOD) != MA)
                continue;
              Instruction *MemI = MUOD->getMemoryInst();
              if (MemI && Visited.insert(MemI).second)
                Q.push(MemI);
            }
          }
        }
      }
    }
  }

  return false;
}

bool areIndependent(Instruction *I1, Instruction *I2, MemorySSA &MSSA) {
  if (isTransitivelyDependent(I1, I2, MSSA) ||
      isTransitivelyDependent(I2, I1, MSSA))
    return false;

  if (!I1->mayWriteToMemory() && !I2->mayWriteToMemory())
    return true;

  MemoryAccess *MA1 = MSSA.getMemoryAccess(I1);
  MemoryAccess *MA2 = MSSA.getMemoryAccess(I2);
  if (!MA1 || !MA2)
    return true;

  if (MemorySSAWalker *W = MSSA.getWalker()) {
    if (W->getClobberingMemoryAccess(MA1) == MA2)
      return false;
    if (W->getClobberingMemoryAccess(MA2) == MA1)
      return false;
  }

  return true;
}

bool areSchedulableTogether(Instruction *I1, Instruction *I2) {
  return I1->getParent() == I2->getParent();
}

void addPack(CandidatePairs &C, const Instruction *I1, const Instruction *I2) {
  std::vector<const Instruction *> Pack;
  Pack.push_back(I1);
  Pack.push_back(I2);
  C.Packs.push_back(std::move(Pack));
}

bool isCandidateStatement(Instruction *I) {
  if (auto *L = dyn_cast<LoadInst>(I))
    return isScalarOrVectorIntOrFP(L->getType());

  if (auto *S = dyn_cast<StoreInst>(I))
    return isScalarOrVectorIntOrFP(S->getValueOperand()->getType());

  if (auto *BO = dyn_cast<BinaryOperator>(I))
    return isScalarOrVectorIntOrFP(BO->getType());

  if (auto *CI = dyn_cast<CallInst>(I)) {
    if (Function *F = CI->getCalledFunction()) {
      if (F->getIntrinsicID() == Intrinsic::fmuladd)
        return true;
    }
  }

  return false;
}

bool legalGoSLPPair(Instruction *I1, Instruction *I2, const DataLayout &DL,
                    AAResults &AA, MemorySSA &MSSA) {
  if (I1 == I2)
    return false;

  if (!areIsomorphic(I1, I2))
    return false;

  if (!areIndependent(I1, I2, MSSA))
    return false;

  if (!areSchedulableTogether(I1, I2))
    return false;

  if (accessesMemory(I1) || accessesMemory(I2)) {
    if (!areAdjacentMemoryAccesses(I1, I2, DL, AA))
      return false;
  }

  return true;
}

CandidatePairs collectCandidatePairs(Function &F, AAResults &AA, MemorySSA &MSSA,
                                     bool debug) {
  CandidatePairs Result;
  Module *M = F.getParent();
  if (!M)
    return Result;

  const DataLayout &DL = M->getDataLayout();
  std::unordered_map<ValuePackKey, uint32_t, ValuePackKeyHash> PackToIdx;

  for (BasicBlock &BB : F) {
    struct IsoBucketKey {
      unsigned Kind = 0; // 0 load, 1 store, 2 binop, 3 call
      unsigned OpcodeOrIntrinsic = 0;
      Type *Ty = nullptr;
      unsigned AddrSpace = 0;
      bool operator==(const IsoBucketKey &O) const {
        return Kind == O.Kind && OpcodeOrIntrinsic == O.OpcodeOrIntrinsic &&
               Ty == O.Ty && AddrSpace == O.AddrSpace;
      }
    };

    struct IsoBucketHash {
      size_t operator()(const IsoBucketKey &K) const noexcept {
        size_t H = std::hash<unsigned>{}(K.Kind);
        H ^= std::hash<unsigned>{}(K.OpcodeOrIntrinsic) + 0x9e3779b9 + (H << 6) +
             (H >> 2);
        H ^= std::hash<Type *>{}(K.Ty) + 0x9e3779b9 + (H << 6) + (H >> 2);
        H ^= std::hash<unsigned>{}(K.AddrSpace) + 0x9e3779b9 + (H << 6) +
             (H >> 2);
        return H;
      }
    };

    std::unordered_map<IsoBucketKey, std::vector<Instruction *>, IsoBucketHash>
        Buckets;

    for (Instruction &I : BB) {
      if (!isCandidateStatement(&I))
        continue;

      IsoBucketKey Key;
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        Key.Kind = 0;
        Key.OpcodeOrIntrinsic = Instruction::Load;
        Key.Ty = LI->getType();
        Key.AddrSpace = LI->getPointerAddressSpace();
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
        Key.Kind = 1;
        Key.OpcodeOrIntrinsic = Instruction::Store;
        Key.Ty = SI->getValueOperand()->getType();
        Key.AddrSpace = SI->getPointerAddressSpace();
      } else if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
        Key.Kind = 2;
        Key.OpcodeOrIntrinsic = BO->getOpcode();
        Key.Ty = BO->getType();
      } else if (auto *CI = dyn_cast<CallInst>(&I)) {
        Key.Kind = 3;
        Key.OpcodeOrIntrinsic = CI->getIntrinsicID();
        Key.Ty = CI->getType();
      } else {
        continue;
      }

      Buckets[Key].push_back(&I);
    }

    for (auto &Entry : Buckets) {
      auto &Stmts = Entry.second;
      const size_t PairBudgetPerBucket = debug ? 1000000 : 4096;
      size_t PairChecks = 0;
      for (size_t I = 0; I < Stmts.size(); ++I) {
        for (size_t J = I + 1; J < Stmts.size(); ++J) {
          if (++PairChecks > PairBudgetPerBucket)
            break;
          Instruction *S1 = Stmts[I];
          Instruction *S2 = Stmts[J];
          if (!legalGoSLPPair(S1, S2, DL, AA, MSSA))
            continue;

          std::vector<const Instruction *> Pack{S1, S2};
          addPackUnique(Result, PackToIdx, Pack);
        }
        if (PairChecks > PairBudgetPerBucket)
          break;
      }
    }
  }

  if (!debug && Result.Packs.size() <= 96)
    widenPacks(Result, DL, AA, MSSA, /*MaxWidth=*/8, PackToIdx);

  // Keep the ILP tractable and deterministic on large functions.
  const size_t MaxPacks = debug ? 256 : 96;
  if (Result.Packs.size() > MaxPacks)
    Result.Packs.resize(MaxPacks);

  rebuildInstToCandidates(Result);
  buildUseMaps(Result);
  buildCircularConflicts(Result, MSSA);

  return Result;
}

void printCandidatePairs(const CandidatePairs &CP) {
  errs() << "===== CandidatePairs =====\n";

  const size_t Limit = 80;
  errs() << "Packs (" << CP.Packs.size() << "): showing first "
         << std::min(Limit, CP.Packs.size()) << "\n";
  for (size_t I = 0; I < CP.Packs.size() && I < Limit; ++I) {
    errs() << "  Pack " << I << ":\n";
    for (const Instruction *Inst : CP.Packs[I]) {
      errs() << "    ";
      if (Inst)
        Inst->print(errs());
      else
        errs() << "<null inst>";
      errs() << "\n";
    }
  }
  if (CP.Packs.size() > Limit)
    errs() << "  ... (" << (CP.Packs.size() - Limit)
           << " more packs elided)\n";

  errs() << "InstToCandidates: " << CP.InstToCandidates.size() << "\n";
  errs() << "VecVecUses keys: " << CP.VecVecUses.size() << "\n";
  errs() << "NonVecPacks: " << CP.NonVecPacks.size() << "\n";
  errs() << "================================\n";
}
