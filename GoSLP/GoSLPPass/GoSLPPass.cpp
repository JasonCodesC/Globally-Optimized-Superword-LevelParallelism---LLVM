#include "CandidatePacks.cpp"
#include "ILP.cpp"
#include "VecGraph.cpp"
#include "PermuteDP.cpp"
#include "Emit.cpp"

#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"


using namespace llvm;

PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    AAResults &AA = FAM.getResult<AAManager>(F);
    auto &MSSAAnalysis = FAM.getResult<MemorySSAAnalysis>(F);
    MemorySSA &MSSA = MSSAAnalysis.getMSSA();

    //There will probably be a loop around all of this for iterative widening.
    CandidatePairs C = collectCandidatePairs(F, AA, MSSA);
    
    if (C.Packs.empty()) {
        return PreservedAnalyses::all();
    }

    VecGraph graph = buildVectorGraph(C);
    ShuffleCost sc = createShuffleCostCalculator(F, TTI, C);
    std::vector<double> PackCost(C.Packs.size(), 0.0);
    for (size_t i = 0; i < C.Packs.size(); ++i) {
        Node n;
        n.PackIdx = static_cast<int>(i);
        n.pack = C.Packs[i];
        double packCost = (double)sc.getPackCost(n);
        double unpackCost = (double)sc.getUnpackCost(n);
        PackCost[i] = packCost + unpackCost;
    }

    std::vector<bool> Chosen = solveILP(C, PackCost, 120);

    // dp
    // emit
    
    return PreservedAnalyses::all();
}