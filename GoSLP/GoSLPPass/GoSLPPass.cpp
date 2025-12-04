// // main pass
// #include "CandidatePacks.hpp"
// #include "ILP.hpp"
// #include "VecGraph.hpp"
// #include "ShuffleCost.hpp" 
// #include "PermuteDP.hpp"
// #include "Emit.hpp"

// #include "llvm/IR/PassManager.h"
// #include "llvm/Analysis/AliasAnalysis.h"
// #include "llvm/Analysis/MemorySSA.h"
// #include "llvm/Analysis/TargetTransformInfo.h"

// #include "llvm/Pass.h"
// #include "llvm/Passes/PassBuilder.h"
// #include "llvm/Passes/PassPlugin.h"


// using namespace llvm;

// class GoSLPPass : public PassInfoMixin<GoSLPPass> {
// public:
//     PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
// };

// PreservedAnalyses GoSLPPass::run(Function &F, FunctionAnalysisManager &FAM) {
//     AAResults &AA = FAM.getResult<AAManager>(F);
//     auto &MSSAAnalysis = FAM.getResult<MemorySSAAnalysis>(F);
//     MemorySSA &MSSA = MSSAAnalysis.getMSSA();
//     TargetTransformInfo &TTI = FAM.getResult<TargetIRAnalysis>(F);

//     bool worked = false;

//     int i = 0;
//     while (true) {
//         i++;
//         errs() << "iter " << i << "\n";
//         CandidatePairs C = collectCandidatePairs(F, AA, MSSA);
//         printCandidatePairs(C);
        
//         if (C.Packs.empty()) {
//             break;
//         }


//         VecGraph G = buildVectorGraph(C);
//         ShuffleCost SC = createShuffleCostCalculator(F, TTI, C);
//         std::vector<double> PackCost(C.Packs.size(), 0.0);
//         for (int i = 0; i < C.Packs.size(); ++i) {
//             Node N;
//             N.PackIdx = i;
//             N.pack = C.Packs[i];

//             InstructionCost pack = SC.getPackCost(N);
//             InstructionCost unpack = SC.getUnpackCost(N);
//             int pack_val = 0;
//             if (pack.isValid()) {
//                 pack_val = pack.getValue().value_or(0);
//             }
//             int unpack_val = 0;
//             if (unpack.isValid()) {
//                 unpack_val = unpack.getValue().value_or(0);
//             }

//             PackCost[i] = static_cast<double>(pack_val + unpack_val);
//         }
        
//         std::vector<bool> Chosen = solveILP(C, PackCost, 120.0);
//         // if (Chosen.size() >= 1) { 
//         //     Chosen[0] = true;
//         // }
//         for (auto it : Chosen) {
//             errs() << it << "\n"; 
//         }
//         for (auto it : PackCost) {
//             errs() << it << "\n"; 
//         }
//         bool AnyChosen = false;
//         for (bool b : Chosen) {
//             if (b) { 
//                 AnyChosen = true; 
//                 break; 
//             }
//         }
//         if (!AnyChosen) {
//             errs() << "line 85";
//             break;
//         }

//         Perms LanePerm = choosePermutationsDP(G, SC);
//         bool Changed = emit(F, C, Chosen, LanePerm);
//         if (!Changed) {
//             errs() << "line 92";
//             break; 
//         }

//         worked |= Changed;
//         break;
//     }
//     if (worked) {
//         return PreservedAnalyses::none();
//     }
//     else {
//         return PreservedAnalyses::all();
//     }
// }

// extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
//     return {
//         LLVM_PLUGIN_API_VERSION,
//         "GoSLPPass",
//         "1.0",
//         [](PassBuilder &PB) {
//             PB.registerPipelineParsingCallback(
//                 [](StringRef Name,
//                    FunctionPassManager &FPM,
//                    ArrayRef<PassBuilder::PipelineElement>) {
//                     if (Name == "GoSLPPass") {
//                         FPM.addPass(GoSLPPass());
//                         return true;
//                     }
//                     return false;
//                 });
//         }
//     };
// }




// GoSLPPass.cpp

#include "CandidatePacks.hpp"
#include "ILP.hpp"
#include "VecGraph.hpp"
#include "ShuffleCost.hpp"
#include "PermuteDP.hpp"
#include "Emit.hpp"

#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/TargetTransformInfo.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormatVariadic.h"

using namespace llvm;


class GoSLPPass : public PassInfoMixin<GoSLPPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

PreservedAnalyses GoSLPPass::run(Function &F, FunctionAnalysisManager &FAM) {
    AAResults &AA = FAM.getResult<AAManager>(F);
    auto &MSSAAnalysis = FAM.getResult<MemorySSAAnalysis>(F);
    MemorySSA &MSSA = MSSAAnalysis.getMSSA();
    TargetTransformInfo &TTI = FAM.getResult<TargetIRAnalysis>(F);

    bool worked = false;
    int iter = 0;

    while (true) {
        ++iter;
        errs() << "\n========== GoSLP iteration #" << iter << " on func " << F.getName() << " ==========\n";

        // 1) Collect legal 2-wide candidate packs
        CandidatePairs C = collectCandidatePairs(F, AA, MSSA);
        printCandidatePairs(C);

        if (C.Packs.empty()) {
            errs() << "No candidate packs, stopping.\n";
            break;
        }

        // 2) Build vectorization graph (for permutations + shuffle modeling)
        VecGraph G = buildVectorGraph(C);

        // 3) Build shuffle/pack/unpack cost model
        ShuffleCost SC = createShuffleCostCalculator(F, TTI, C);

        // 4) Build per-pack cost vector for ILP
        //
        //    We want: minimize sum_i PackCost[i] * x_i
        //    with PackCost[i] negative when vectorizing that pack is beneficial.
        //
        //    Simple model:
        //      PackCost[i] = pack_cost + unpack_cost - benefitPerLane * width
        //
        //    benefitPerLane is a big constant so we will almost always
        //    prefer vectorization, subject to conflicts, which is
        //    useful for correctness testing / debugging.
        std::vector<double> PackCost(C.Packs.size(), 0.0);
        const double benefitPerLane = 5;  // tune as you like

        for (int i = 0; i < static_cast<int>(C.Packs.size()); ++i) {
            Node N;
            N.PackIdx = i;
            N.pack = C.Packs[i];

            InstructionCost packCostIC   = SC.getPackCost(N);
            InstructionCost unpackCostIC = SC.getUnpackCost(N);

            int pack_val   = (packCostIC.isValid()   ? packCostIC.getValue().value_or(0)   : 0);
            int unpack_val = (unpackCostIC.isValid() ? unpackCostIC.getValue().value_or(0) : 0);

            int width = static_cast<int>(N.pack.size()); // should be 2 here
            double vecBenefit = benefitPerLane * static_cast<double>(width);

            // Smaller is better; negative means "good to take".
            PackCost[i] = static_cast<double>(pack_val + unpack_val) - vecBenefit;
        }

        // Debug: prinlt costs
        errs() << "==================== Pack Costs ====================\n";
        for (size_t i = 0; i < PackCost.size(); ++i) {
            errs() << formatv("  Pack {0,3} : {1,8:F2}\n", i, PackCost[i]);
        }
        errs() << "===================================================\n";

        // 5) Solve ILP over packs
        std::vector<bool> Chosen = solveILP(C, PackCost, /*TimeLimitSeconds=*/120.0);

        
        errs() << "================= Chosen Packs (by ILP) ===============\n";
        bool AnyChosen = false;
        for (size_t i = 0; i < Chosen.size(); ++i) {
            errs() << "  Pack " << i << ": " << (Chosen[i] ? "YES" : "NO") << "\n";
            if (Chosen[i])
                AnyChosen = true;
        }
        errs() << "===================================================\n";

        if (!AnyChosen) {
            errs() << "ILP chose no packs; stopping.\n";
            break;
        }

        // 6) Choose lane permutations for chosen packs (GoSLP DP)
        Perms LanePerm = choosePermutationsDP(G, SC);

        // 7) Emit vector IR for chosen packs
        bool Changed = emit(F, C, Chosen, LanePerm);
        if (!Changed) {
            errs() << "Emit produced no changes; stopping.\n";
            break;
        }

        worked |= Changed;
        break;
    }

    if (worked) {
        return PreservedAnalyses::none();
    } else {
        return PreservedAnalyses::all();
    }
}

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "GoSLPPass",
        "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "GoSLPPass") {
                        FPM.addPass(GoSLPPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}
