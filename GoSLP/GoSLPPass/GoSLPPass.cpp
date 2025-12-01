// main pass
#pragma once
#include "CandidatePacks.cpp"
#include "ILP.cpp"
#include "VecGraph.cpp"
#include "ShuffleCost.cpp" 
#include "PermuteDP.cpp"
#include "Emit.cpp"

#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/TargetTransformInfo.h"


using namespace llvm;

PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    AAResults &AA = FAM.getResult<AAManager>(F);
    auto &MSSAAnalysis = FAM.getResult<MemorySSAAnalysis>(F);
    MemorySSA &MSSA = MSSAAnalysis.getMSSA();
    TargetTransformInfo &TTI = FAM.getResult<TargetIRAnalysis>(F);

    bool worked = false;


    while (true) {
        CandidatePairs C = collectCandidatePairs(F, AA, MSSA);
        if (C.Packs.empty()) {
            break;
        }


        VecGraph G = buildVectorGraph(C);
        ShuffleCost SC = createShuffleCostCalculator(F, TTI, C);
        std::vector<double> PackCost(C.Packs.size(), 0.0);
        for (int i = 0; i < C.Packs.size(); ++i) {
            Node N;
            N.PackIdx = i;
            N.pack = C.Packs[i];

            InstructionCost pack = SC.getPackCost(N);
            InstructionCost unpack = SC.getUnpackCost(N);
            int pack_val = 0;
            if (pack.isValid()) {
                pack_val = pack.getValue();
            }
            int unpack_val = 0;
            if (unpack.isValid()) {
                unpack_val = unpack.getValue();
            }
            PackCost[i] = static_cast<double>(pack_val + unpack_val);
        }
        std::vector<bool> Chosen = solveILP(C, PackCost, 120.0);

        bool AnyChosen = false;
        for (bool b : Chosen) {
            if (b) { 
                AnyChosen = true; 
                break; 
            }
        }
        if (!AnyChosen) {
            break;
        }

        Perms LanePerm = choosePermutationsDP(G, SC);
        bool Changed = emit(F, C, Chosen, LanePerm);
        if (!Changed) {
            break; 
        }

        worked |= Changed;
    }
    if (worked) {
        return PreservedAnalyses::none();
    }
    else {
        return PreservedAnalyses::all();
    }
}
