// shuffleCost
#include "ShuffleCost.hpp"

// get element type from instruction
Type* ShuffleCost::getElementType(const Instruction *I) const {
    if (auto *SI = dyn_cast<StoreInst>(I))
        return SI->getValueOperand()->getType();
    return I->getType();
}
    
// compute lane mapping. DstPack[i] uses which lane from SrcPack?
// returns: laneMap[dstLane] = srcLane (or -1 if no mapping)
std::vector<int> ShuffleCost::computeLaneMap(
    const std::vector<const Instruction*>& SrcPack,
    const std::vector<const Instruction*>& DstPack,
    unsigned operandIdx) const {
    
    std::vector<int> laneMap(DstPack.size(), -1);
    
    for (size_t dstLane = 0; dstLane < DstPack.size(); ++dstLane) {
        const Instruction *DstI = DstPack[dstLane];
        
        // check if operand at operandIdx comes from SrcPack
        if (operandIdx < DstI->getNumOperands()) {
            if (auto *OpI = dyn_cast<Instruction>(DstI->getOperand(operandIdx))) {
                // find which lane in SrcPack
                for (size_t srcLane = 0; srcLane < SrcPack.size(); ++srcLane) {
                    if (SrcPack[srcLane] == OpI) {
                        laneMap[dstLane] = srcLane;
                        break;
                    }
                }
            }
        }
    }
    
    return laneMap;
}
    
// check if lane map needs shuffle
bool ShuffleCost::needsShuffle(const std::vector<int>& laneMap) const {
    for (size_t i = 0; i < laneMap.size(); ++i) {
        if (laneMap[i] != -1 && laneMap[i] != (int)i)
            return true;
    }
    return false;
}
    
// check if laneMap is valid (has at least one mapping)
bool ShuffleCost::hasMapping(const std::vector<int>& laneMap) const {
    for (int lane : laneMap) {
        if (lane != -1) return true;
    }
    return false;
}

ShuffleCost::ShuffleCost(const TargetTransformInfo *tti, const DataLayout *dl, 
            const CandidatePairs *C)
    : TTI(tti), DL(dl), Candidates(C) {}

// pack cost
InstructionCost ShuffleCost::getPackCost(const Node &N) const {
    Type *ElemTy = getElementType(N.pack[0]);
    auto *VecTy = FixedVectorType::get(ElemTy, N.pack.size());
    
    InstructionCost cost = 0;
    for (size_t i = 0; i < N.pack.size(); ++i) {
        cost += TTI->getVectorInstrCost(
            Instruction::InsertElement,
            VecTy,
            TTI::TCK_RecipThroughput,
            i,
            nullptr,
            nullptr
        );
    }
    return cost;
}
    
// unpack cost
InstructionCost ShuffleCost::getUnpackCost(const Node &N) const {
    Type *ElemTy = getElementType(N.pack[0]);
    auto *VecTy = FixedVectorType::get(ElemTy, N.pack.size());
    
    InstructionCost cost = 0;
    
    for (size_t lane = 0; lane < N.pack.size(); ++lane) {
        const Instruction *I = N.pack[lane];
        
        for (const User *U : I->users()) {
            if (auto *UI = dyn_cast<Instruction>(U)) {
                // check if this user is in any pack (vectorized)
                auto It = Candidates->InstToCandidates.find(UI);
                if (It == Candidates->InstToCandidates.end()) {
                    // scalar user -> needs extract
                    cost += TTI->getVectorInstrCost(
                        Instruction::ExtractElement,
                        VecTy,
                        TTI::TCK_RecipThroughput,
                        lane,
                        nullptr,
                        nullptr
                    );
                }
            }
        }
    }
    
    return cost;
}
    
// shuffle cost between two packs
InstructionCost ShuffleCost::getShuffleCost(const Node &Src, const Node &Dst) const {
    InstructionCost totalCost = 0;
    
    // find all unique operands that connect Src to Dst
    std::set<unsigned> operandIndices;
    
    for (size_t dstLane = 0; dstLane < Dst.pack.size(); ++dstLane) {
        const Instruction *DstI = Dst.pack[dstLane];
        
        for (unsigned opIdx = 0; opIdx < DstI->getNumOperands(); ++opIdx) {
            if (auto *OpI = dyn_cast<Instruction>(DstI->getOperand(opIdx))) {
                // check if OpI is in Src pack
                bool inSrcPack = std::find(Src.pack.begin(), Src.pack.end(), OpI) 
                                    != Src.pack.end();
                if (inSrcPack) {
                    operandIndices.insert(opIdx);
                }
            }
        }
    }
    
    // compute cost for each operand
    for (unsigned opIdx : operandIndices) {
        auto laneMap = computeLaneMap(Src.pack, Dst.pack, opIdx);
        
        if (hasMapping(laneMap) && needsShuffle(laneMap)) {
            Type *ElemTy = getElementType(Src.pack[0]);
            auto *VecTy = FixedVectorType::get(ElemTy, Src.pack.size());
            
            totalCost += TTI->getShuffleCost(
                TargetTransformInfo::SK_PermuteSingleSrc,
                VecTy,
                VecTy,
                laneMap
            );
        }
    }
    
    return totalCost;
}
    
// total shuffle cost for entire graph
InstructionCost ShuffleCost::getTotalShuffleCost(const VecGraph& G) const {
    InstructionCost total = 0;
    
    for (const Node& N : G.items) {
        for (int useIdx : N.Uses) {
            total += getShuffleCost(N, G.items[useIdx]);
        }
    }
    
    return total;
}
    
// cost breakdown for debugging
std::map<std::pair<int, int>, InstructionCost> 
ShuffleCost::getShuffleCostBreakdown(const VecGraph& G) const {
    std::map<std::pair<int, int>, InstructionCost> costs;
    
    for (const Node& N : G.items) {
        for (int useIdx : N.Uses) {
            auto edge = std::make_pair(N.PackIdx, useIdx);
            costs[edge] = getShuffleCost(N, G.items[useIdx]);
        }
    }
    
    return costs;
}

// helper to create calculator from Function
ShuffleCost createShuffleCostCalculator(Function &F, TargetTransformInfo &TTI, CandidatePairs &C) {
    const DataLayout &DL = F.getParent()->getDataLayout();
    return ShuffleCost(&TTI, &DL, &C);
}
