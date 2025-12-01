#pragma once
#include "VecGraph.hpp"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <map>
#include <set>

using namespace llvm;

class ShuffleCost {
private:
    const TargetTransformInfo *TTI;
    const DataLayout *DL;
    const CandidatePairs *Candidates;
    
    // get element type from instruction
    Type* getElementType(const Instruction *I) const;
    
    // compute lane mapping. DstPack[i] uses which lane from SrcPack?
    // returns: laneMap[dstLane] = srcLane (or -1 if no mapping)
    std::vector<int> computeLaneMap(
        const std::vector<const Instruction*>& SrcPack,
        const std::vector<const Instruction*>& DstPack,
        unsigned operandIdx) const;
    
    // check if lane map needs shuffle
    bool needsShuffle(const std::vector<int>& laneMap) const;
    
    // check if laneMap is valid (has at least one mapping)
    bool hasMapping(const std::vector<int>& laneMap) const;

public:
    ShuffleCost(const TargetTransformInfo *tti, const DataLayout *dl, 
                const CandidatePairs *C);
    
    // pack cost
    InstructionCost getPackCost(const Node &N) const;
    
    // unpack cost
    InstructionCost getUnpackCost(const Node &N) const;
    
    // shuffle cost between two packs
    InstructionCost getShuffleCost(const Node &Src, const Node &Dst) const;
    
    // total shuffle cost for entire graph
    InstructionCost getTotalShuffleCost(const VecGraph& G) const;
    
    // cost breakdown for debugging
    std::map<std::pair<int, int>, InstructionCost> 
    getShuffleCostBreakdown(const VecGraph& G) const;
};

// helper to create calculator from Function
ShuffleCost createShuffleCostCalculator(Function &F, TargetTransformInfo &TTI, CandidatePairs &C);
