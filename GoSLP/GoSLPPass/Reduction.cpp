#include "Reduction.hpp"
#include "CandidatePacks.hpp"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

namespace {

struct ReductionCandidate {
  Instruction *Root = nullptr;
  unsigned Opcode = 0;
  Type *Ty = nullptr;
  FastMathFlags FMF;
  bool IsFloating = false;
  SmallVector<Value *, 16> Terms;
  SmallVector<Instruction *, 16> Nodes;
};

static double toDouble(InstructionCost C) {
  if (!C.isValid())
    return 0.0;
  return static_cast<double>(C.getValue());
}

static bool isReductionOpcode(unsigned Opcode) {
  return Opcode == Instruction::Add || Opcode == Instruction::FAdd;
}

static bool collectReductionTerms(Value *V, BasicBlock *BB, unsigned Opcode,
                                  Type *Ty,
                                  SmallPtrSetImpl<Instruction *> &Visited,
                                  SmallVectorImpl<Value *> &Terms,
                                  SmallVectorImpl<Instruction *> &Nodes) {
  auto *I = dyn_cast<Instruction>(V);
  if (!I || I->getParent() != BB || I->getType() != Ty ||
      I->getOpcode() != Opcode) {
    Terms.push_back(V);
    return true;
  }

  if (!Visited.insert(I).second) {
    Terms.push_back(V);
    return true;
  }

  Nodes.push_back(I);
  if (!collectReductionTerms(I->getOperand(0), BB, Opcode, Ty, Visited, Terms,
                             Nodes))
    return false;
  if (!collectReductionTerms(I->getOperand(1), BB, Opcode, Ty, Visited, Terms,
                             Nodes))
    return false;
  return true;
}

static unsigned chooseReductionWidth(size_t Terms) {
  if (Terms >= 4)
    return 4;
  if (Terms >= 2)
    return 2;
  return 0;
}

static bool isAArch64Target(const Module &M) {
  Triple T(M.getTargetTriple());
  return T.isAArch64();
}

static Value *buildReductionValue(IRBuilder<> &B, const ReductionCandidate &C,
                                  unsigned Width, const DataLayout &DL,
                                  bool &UsedVectorLoad) {
  auto *VecTy = FixedVectorType::get(C.Ty, Width);
  UsedVectorLoad = false;
  Value *Vec = nullptr;

  // Prefer direct vector loads for contiguous reduction leaves.
  SmallVector<std::pair<int64_t, LoadInst *>, 8> LoadTerms;
  bool AllLoads = true;
  const Value *BaseObj = nullptr;
  int64_t ElemSize = 0;
  for (unsigned I = 0; I < Width; ++I) {
    auto *LI = dyn_cast<LoadInst>(C.Terms[I]);
    if (!LI) {
      AllLoads = false;
      break;
    }

    const Value *Base = nullptr;
    int64_t Off = 0;
    if (!getAddrBaseAndOffset(LI, DL, Base, Off)) {
      AllLoads = false;
      break;
    }

    int64_t CurElemSize = static_cast<int64_t>(DL.getTypeStoreSize(C.Ty));
    if (I == 0) {
      BaseObj = Base;
      ElemSize = CurElemSize;
    } else if (BaseObj != Base || ElemSize != CurElemSize) {
      AllLoads = false;
      break;
    }

    LoadTerms.push_back({Off, LI});
  }

  if (AllLoads && !LoadTerms.empty()) {
    llvm::stable_sort(LoadTerms, [](auto &A, auto &B) { return A.first < B.first; });
    bool Contiguous = true;
    for (unsigned I = 1; I < LoadTerms.size(); ++I) {
      if (LoadTerms[I].first - LoadTerms[I - 1].first != ElemSize) {
        Contiguous = false;
        break;
      }
    }

    if (Contiguous) {
      LoadInst *BaseLoad = LoadTerms.front().second;
      Value *BasePtr = BaseLoad->getPointerOperand();
      // Opaque pointer mode: CreateLoad takes element type explicitly.
      Vec = B.CreateLoad(VecTy, BasePtr, "goslp.red.vload");
      UsedVectorLoad = true;
    }
  }

  if (!Vec) {
    Vec = UndefValue::get(VecTy);
    for (unsigned I = 0; I < Width; ++I) {
      Vec = B.CreateInsertElement(Vec, C.Terms[I], B.getInt32(I),
                                  "goslp.red.ins");
    }
  }

  if (C.Opcode == Instruction::Add) {
    return B.CreateAddReduce(Vec);
  }

  Value *Acc = ConstantFP::get(C.Ty, 0.0);
  return B.CreateFAddReduce(Acc, Vec);
}

static double estimateReductionCost(const ReductionCandidate &C, unsigned Width,
                                    TargetTransformInfo &TTI,
                                    const DataLayout &DL) {
  auto CostKind = TargetTransformInfo::TCK_RecipThroughput;
  auto *VecTy = FixedVectorType::get(C.Ty, Width);

  double ScalarAddCost =
      toDouble(TTI.getArithmeticInstrCost(C.Opcode, C.Ty, CostKind));
  double ScalarTotal = ScalarAddCost * static_cast<double>(C.Nodes.size());

  bool CanUseContiguousLoad = true;
  const Value *BaseObj = nullptr;
  int64_t ElemSize = 0;
  SmallVector<int64_t, 8> Offsets;
  Offsets.reserve(Width);
  for (unsigned I = 0; I < Width; ++I) {
    auto *LI = dyn_cast<LoadInst>(C.Terms[I]);
    if (!LI) {
      CanUseContiguousLoad = false;
      break;
    }
    const Value *Base = nullptr;
    int64_t Off = 0;
    if (!getAddrBaseAndOffset(LI, DL, Base, Off)) {
      CanUseContiguousLoad = false;
      break;
    }
    int64_t CurElemSize = static_cast<int64_t>(DL.getTypeStoreSize(C.Ty));
    if (I == 0) {
      BaseObj = Base;
      ElemSize = CurElemSize;
    } else if (BaseObj != Base || ElemSize != CurElemSize) {
      CanUseContiguousLoad = false;
      break;
    }
    Offsets.push_back(Off);
  }
  if (CanUseContiguousLoad) {
    llvm::stable_sort(Offsets);
    for (unsigned I = 1; I < Offsets.size(); ++I) {
      if (Offsets[I] - Offsets[I - 1] != ElemSize) {
        CanUseContiguousLoad = false;
        break;
      }
    }
  }

  double PackCost = 0.0;
  if (!CanUseContiguousLoad) {
    for (unsigned I = 0; I < Width; ++I) {
      PackCost += toDouble(TTI.getVectorInstrCost(Instruction::InsertElement, VecTy,
                                                  CostKind, I, nullptr,
                                                  nullptr));
    }
  }

  std::optional<FastMathFlags> FMF;
  if (C.Opcode == Instruction::FAdd)
    FMF = C.FMF;

  double ReduceCost =
      toDouble(TTI.getArithmeticReductionCost(C.Opcode, VecTy, FMF, CostKind));

  // Tail terms remain scalar additions.
  size_t TailTerms = C.Terms.size() > Width ? C.Terms.size() - Width : 0;
  double TailCost = ScalarAddCost * static_cast<double>(TailTerms);

  return (PackCost + ReduceCost + TailCost) - ScalarTotal;
}

static bool emitReduction(const ReductionCandidate &C, unsigned Width) {
  if (!C.Root || Width < 2 || C.Terms.size() < Width)
    return false;

  IRBuilder<> B(C.Root);
  const DataLayout &DL = C.Root->getFunction()->getParent()->getDataLayout();
  bool UsedVectorLoad = false;
  Value *Red = buildReductionValue(B, C, Width, DL, UsedVectorLoad);
  Value *Result = Red;

  for (size_t I = Width; I < C.Terms.size(); ++I) {
    if (C.Opcode == Instruction::Add) {
      Result = B.CreateAdd(Result, C.Terms[I], "goslp.red.tail");
    } else {
      auto *F = cast<Instruction>(B.CreateFAdd(Result, C.Terms[I],
                                               "goslp.red.tail"));
      F->setFastMathFlags(C.FMF);
      Result = F;
    }
  }

  C.Root->replaceAllUsesWith(Result);

  return true;
}

} // namespace

bool runReductionAwareGoSLP(Function &F, TargetTransformInfo &TTI,
                            const DataLayout &DL, bool Debug) {
  (void)DL;
  Module *M = F.getParent();
  if (!M || !isAArch64Target(*M))
    return false;

  bool Changed = false;
  SmallPtrSet<Instruction *, 32> Claimed;

  for (BasicBlock &BB : F) {
    for (Instruction &IRef : BB) {
      auto *I = dyn_cast<BinaryOperator>(&IRef);
      if (!I)
        continue;

      if (!isReductionOpcode(I->getOpcode()))
        continue;

      if (Claimed.contains(I))
        continue;

      if (!I->getType()->isIntegerTy() && !I->getType()->isFloatingPointTy())
        continue;

      if (I->getOpcode() == Instruction::FAdd && !I->hasAllowReassoc())
        continue;

      bool HasAddUser = false;
      for (User *U : I->users()) {
        auto *UI = dyn_cast<Instruction>(U);
        if (!UI || UI->getParent() != &BB)
          continue;
        if (UI->getOpcode() == I->getOpcode()) {
          HasAddUser = true;
          break;
        }
      }
      if (HasAddUser)
        continue;

      ReductionCandidate Cand;
      Cand.Root = I;
      Cand.Opcode = I->getOpcode();
      Cand.Ty = I->getType();
      Cand.IsFloating = I->getOpcode() == Instruction::FAdd;
      if (Cand.IsFloating)
        Cand.FMF = I->getFastMathFlags();

      SmallPtrSet<Instruction *, 32> Visited;
      if (!collectReductionTerms(Cand.Root, &BB, Cand.Opcode, Cand.Ty, Visited,
                                 Cand.Terms, Cand.Nodes))
        continue;

      // Guardrails for reduction graph complexity.
      if (Cand.Terms.size() < 2 || Cand.Terms.size() > 16)
        continue;
      if (Cand.Nodes.size() < 2)
        continue;

      unsigned Width = chooseReductionWidth(Cand.Terms.size());
      if (Width < 2)
        continue;

      // Cost guardrail for reduction vectorization.
      double DeltaCost = estimateReductionCost(Cand, Width, TTI, DL);
      // Allow a small positive margin because reduction lowering quality on
      // AArch64 can be better than IR-level scalarized cost estimates.
      if (DeltaCost > 2.0)
        continue;

      if (!emitReduction(Cand, Width))
        continue;

      for (Instruction *N : Cand.Nodes)
        Claimed.insert(N);

      Changed = true;
      if (Debug) {
        errs() << "[GoSLP-Reduction] Vectorized add reduction in function "
               << F.getName() << " with width " << Width << "\n";
      }
    }
  }

  return Changed;
}
