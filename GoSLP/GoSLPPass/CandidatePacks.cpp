/*

CanditdatePacks.cpp

This file collects legal candidate packs via the methods and constraints specified in the GoSLP paper

*/
#pragma once
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

//   errs() << "got here 1\n";

//   if (!ElemTy->isSized())
//     return false;

//   errs() << "got here 2\n";

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
  errs() << "Base 1 : " << *Base1 << " 2: " << *Base2 << "\n";
  if (!areLoadsEquivalent(I1, I2)) {
    return false;
  }
  // if (Base1 != Base2) {
  //   return false;
  // }
  errs() << "got through this 22\n";
  // ensure they are different objects
  // if (AA.isNoAlias(MemoryLocation::get(I1), MemoryLocation::get(I2))) {
  //   return false;
  // }

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
  errs() << "got through this elemty\n";
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

    // push users to q
    for (User *U : Cur->users()) {
      if (auto *UI = dyn_cast<Instruction>(U)) {
        if (Visited.insert(UI).second) {
          errs() << "adding user: " << *U << "\n";
          Q.push(UI);
        }
      }
    }

    // // push users to q
    // if (MemoryAccess *MA = MSSA.getMemoryAccess(Cur)) {
    //   for (User *MU : MA->users()) {
    //     if (auto *MUOD = dyn_cast<MemoryUseOrDef>(MU)) {
    //       Instruction *MemI = MUOD->getMemoryInst();
    //       if (Visited.insert(MemI).second) {
    //         errs() << "adding user: " << *MemI << "\n";
    //         Q.push(MemI);
    //       }
    //     }
    //   }
    // }
  }

  return false;
}

// ensure no dependence either way.
bool areIndependent(Instruction *I1, Instruction *I2, MemorySSA &MSSA) {
  if (isTransitivelyDependent(I1, I2, MSSA)) {
    return false;
  }
  if (isTransitivelyDependent(I2, I1, MSSA)) {
    return false;
  }
  return true;
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

  return false;
}

// Check all goSLP §3.1 constraints for a *pair* (Si, Sj) in the same BB.
bool legalGoSLPPair(Instruction *I1, Instruction *I2, const DataLayout &DL,
    AAResults &AA, MemorySSA &MSSA) {

  errs() << "I1: " << *I1 << " I2: " << *I2 << "\n";
  if (I1 == I2) {
    return false;
  }
  errs() << "got1\n";
  if (!areIsomorphic(I1, I2)) {
    return false;
  }
  errs() << "got2\n";
  if (!areIndependent(I1, I2, MSSA)) {
    return false;
  }
  errs() << "got3\n";
  if (!areSchedulableTogether(I1, I2)) {
    return false;
  }
  errs() << "got4\n";
  if (accessesMemory(I1) || accessesMemory(I2)) {
    if (!areAdjacentMemoryAccesses(I1, I2, DL, AA)) {
      return false;
    }
  }
  errs() << "got5\n";

  return true;
}


// main function
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
    errs() << "Looking at basic block " << BB << "\n";
    for (Instruction &I : BB) {
      if (!isCandidateStatement(&I)) {
        continue;
      }
      errs() << "looking at instruction " << I << "\n";
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
      errs() << "adding to stmtsinbb\n";
      StmtsInBB.push_back(&I);
    }
    const uint32_t N = StmtsInBB.size();
    errs() << "size " << StmtsInBB.size() << "\n";
    if (N < 2) {
      continue;
    }
    for (uint32_t i = 0; i < N; ++i) {
      Instruction *I1 = StmtsInBB[i];
      for (uint32_t j = i + 1; j < N; ++j) {
        Instruction *I2 = StmtsInBB[j];
        if (!legalGoSLPPair(I1, I2, DL, AA, MSSA)) {
          continue;
        }
        errs() << "Calling add pack\n";
        addPack(Result, I1, I2);
      }
    }
  }

  return Result;
}


void printCandidatePairs(const CandidatePairs &CP) {
    errs() << "===== CandidatePairs =====\n";

    // ---- Print Packs ----
    errs() << "Packs (" << CP.Packs.size() << "):\n";
    for (size_t i = 0; i < CP.Packs.size(); ++i) {
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

    // ---- Print InstToCandidates ----
    errs() << "InstToCandidates (" << CP.InstToCandidates.size() << "):\n";
    for (const auto &Entry : CP.InstToCandidates) {
        const Instruction *Inst = Entry.first;
        const std::vector<CandidateId> &Candidates = Entry.second;

        errs() << "  Instruction: ";
        if (Inst)
            Inst->print(errs());
        else
            errs() << "<null inst>";
        errs() << "\n";

        errs() << "    Candidates (" << Candidates.size() << "):\n";
        for (const CandidateId &CID : Candidates) {
            errs() << "      CandidateId { Width=" << CID.Width
                   << ", Index=" << CID.Index << " }\n";
        }
    }

    errs() << "==========================\n";
}
