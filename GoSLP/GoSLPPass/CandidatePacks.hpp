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

struct ValuePackKey {
  std::vector<const Value *> Lanes;
  bool operator==(const ValuePackKey &Other) const { return Lanes == Other.Lanes; }
};

struct ValuePackKeyHash {
  size_t operator()(const ValuePackKey &K) const noexcept {
    size_t H = 1469598103934665603ULL;
    for (const Value *V : K.Lanes) {
      size_t P = std::hash<const Value *>{}(V);
      H ^= P;
      H *= 1099511628211ULL;
    }
    return H;
  }
};

struct LaneUseInfo {
  bool HasOutsideUse = false;
  // Map scalar user -> vector packs that can vectorize this use.
  std::unordered_map<const Instruction *, std::vector<uint32_t>> UserToVectorUses;
};

struct CandidatePairs {
  std::vector<std::vector<const Instruction *>> Packs;
  std::unordered_map<const Instruction *, std::vector<CandidateId>> InstToCandidates;

  // Candidate vector pack -> vectorized user packs that can consume it.
  std::unordered_map<uint32_t, std::vector<uint32_t>> VecVecUses;

  // Non-vectorizable operand packs and their vectorized users.
  std::vector<ValuePackKey> NonVecPacks;
  std::unordered_map<ValuePackKey, uint32_t, ValuePackKeyHash> NonVecPackToIndex;
  std::unordered_map<uint32_t, std::vector<uint32_t>> NonVecVecUses;

  // Per pack/lane extraction analysis data.
  std::vector<std::vector<LaneUseInfo>> LaneUses;

  // Circular dependency conflicts between candidate packs.
  std::vector<std::vector<uint32_t>> CircularConflicts;
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
CandidatePairs collectCandidatePairs(Function &F, AAResults &AA, MemorySSA &MSSA, bool debug);


void printCandidatePairs(const CandidatePairs &CP);
