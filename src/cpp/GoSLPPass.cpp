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
    // VecGraph
    // Shuffle Cost
    // ILP
    // DP
    // Emit
}