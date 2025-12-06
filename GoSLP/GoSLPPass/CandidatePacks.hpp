#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/ArrayRef.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <utility>
#include <queue>

using namespace llvm;

struct CandidateId {
  uint32_t Width;  // current pack width
  uint32_t Index;  // index
};

struct CandidatePairs {
  std::vector<std::vector<const Instruction *>> Packs;
  std::unordered_map<const Instruction *, std::vector<CandidateId>> InstToCandidates;
};

bool accessesMemory(const Instruction *I);
bool isScalarOrVectorIntOrFP(Type *Ty);
bool areIsomorphic(const Instruction *I1, const Instruction *I2);
bool getAddrBaseAndOffset(const Instruction *I, const DataLayout &DL,
        const Value *&Base, int64_t &ByteOffset);
bool areAdjacentMemoryAccesses(const Instruction *I1, const Instruction *I2,
  const DataLayout &DL, AAResults &AA);
bool isTransitivelyDependent(Instruction *From, Instruction *To, MemorySSA &MSSA);
bool areIndependent(Instruction *I1, Instruction *I2, MemorySSA &MSSA);
bool areSchedulableTogether(Instruction *I1, Instruction *I2);
void addPack(CandidatePairs &C, const Instruction *I1, const Instruction *I2);
bool isCandidateStatement(Instruction *I);
bool legalGoSLPPair(Instruction *I1, Instruction *I2, const DataLayout &DL,
    AAResults &AA, MemorySSA &MSSA);
CandidatePairs collectCandidatePairs(Function &F, AAResults &AA, MemorySSA &MSSA);


void printCandidatePairs(const CandidatePairs &CP);
