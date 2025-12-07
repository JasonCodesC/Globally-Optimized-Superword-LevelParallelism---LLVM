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
    const DataLayout &DL = F.getParent()->getDataLayout();

    bool worked = false;
    int iter = 0;

    while (true) {
        ++iter;
        errs() << "\n========== GoSLP iteration #" << iter << " on func " << F.getName() << " ==========\n";

        // 1) Collect legal 2-wide candidate packs
        CandidatePairs C = collectCandidatePairs(F, AA, MSSA);
      //  printCandidatePairs(C);

        if (C.Packs.empty()) {
            errs() << "No candidate packs, stopping.\n";
            break;
        }

        // 2) Build vectorization graph (for permutations + shuffle modeling)
        VecGraph G = buildVectorGraph(C);

        // 3) Build shuffle/pack/unpack cost model
        ShuffleCost SC = createShuffleCostCalculator(F, TTI, C);

        // 4) Build per-pack cost vector for ILP
        std::vector<double> PackCost(C.Packs.size(), 0.0);

        auto estimateOpBenefit = [&](const Instruction *I, unsigned Width) -> double {
            if (!I)
                return 0.0;
            Type *ElemTy = I->getType();
            // Loads/stores use value type rather than ptr type.
            if (auto *SI = dyn_cast<StoreInst>(I))
                ElemTy = SI->getValueOperand()->getType();
            if (!ElemTy || !ElemTy->isIntegerTy() && !ElemTy->isFloatingPointTy())
                return 0.0;

            InstructionCost ScalarCost = 0;
            InstructionCost VectorCost = 0;
            auto CostKind = TargetTransformInfo::TCK_RecipThroughput;

            if (isa<BinaryOperator>(I)) {
                auto *VecTy = FixedVectorType::get(ElemTy, Width);
                unsigned Opc = I->getOpcode();
                ScalarCost = TTI.getArithmeticInstrCost(Opc, ElemTy, CostKind);
                VectorCost = TTI.getArithmeticInstrCost(Opc, VecTy, CostKind);
            } 
            else if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
                auto *VecTy = FixedVectorType::get(ElemTy, Width);
                Align Alignment = DL.getPrefTypeAlign(ElemTy);
                if (isa<LoadInst>(I)) {
                    ScalarCost = TTI.getMemoryOpCost(Instruction::Load, ElemTy, Alignment, 0, CostKind);
                    VectorCost = TTI.getMemoryOpCost(Instruction::Load, VecTy, Alignment, 0, CostKind);
                } 
                else {
                    ScalarCost = TTI.getMemoryOpCost(Instruction::Store, ElemTy, Alignment, 0, CostKind);
                    VectorCost = TTI.getMemoryOpCost(Instruction::Store, VecTy, Alignment, 0, CostKind);
                }
            } 
            else {
                return 0.0;
            }

            auto scalarVal = ScalarCost.isValid() ? static_cast<double>(ScalarCost.getValue()) : 0.0;
            auto vectorVal = VectorCost.isValid() ? static_cast<double>(VectorCost.getValue()) : 0.0;
            return scalarVal * static_cast<double>(Width) - vectorVal;
        };

        for (int i = 0; i < static_cast<int>(C.Packs.size()); ++i) {
            Node N;
            N.PackIdx = i;
            N.pack = C.Packs[i];

            InstructionCost packCostIC = SC.getPackCost(N);
            InstructionCost unpackCostIC = SC.getUnpackCost(N);

            int pack_val = packCostIC.isValid() ? static_cast<int>(packCostIC.getValue()) : 0;
            int unpack_val = unpackCostIC.isValid() ? static_cast<int>(unpackCostIC.getValue()) : 0;

            int width = static_cast<int>(N.pack.size());
            double vecBenefit = estimateOpBenefit(N.pack[0], width);
            double adjustedBenefit = vecBenefit;
            if (adjustedBenefit <= 0.0) {
                adjustedBenefit = 1.5 * static_cast<double>(width);
            }

            PackCost[i] = static_cast<double>(pack_val + unpack_val) - adjustedBenefit;
        }

        // errs() << "==================== Pack Costs ====================\n";
        // const size_t CostPrintLimit = 64;
        // for (size_t i = 0; i < PackCost.size() && i < CostPrintLimit; ++i) {
        //     errs() << formatv("  Pack {0,3} : {1,8:F2}\n", i, PackCost[i]);
        // }
        // if (PackCost.size() > CostPrintLimit)
        //     errs() << "  ... (" << (PackCost.size() - CostPrintLimit) << " more costs elided)\n";
        // errs() << "===================================================\n";

        // 5) Solve ILP over packs
        std::vector<bool> Chosen = solveILP(C, PackCost, /*TimeLimitSeconds=*/1200.0);

        
        // errs() << "================= Chosen Packs (by ILP) ===============\n";
        // bool AnyChosen = false;
        // size_t chosenPrinted = 0;
        // const size_t ChosenPrintLimit = 64;
        // for (size_t i = 0; i < Chosen.size(); ++i) {
        //     if (chosenPrinted < ChosenPrintLimit || Chosen[i]) {
        //         errs() << "  Pack " << i << ": " << (Chosen[i] ? "YES" : "NO") << "\n";
        //         ++chosenPrinted;
        //     }
        //     if (Chosen[i])
        //         AnyChosen = true;
        //     if (chosenPrinted == ChosenPrintLimit && i + 1 < Chosen.size()) {
        //         errs() << "  ... (" << (Chosen.size() - ChosenPrintLimit) << " more entries elided)\n";
        //         break;
        //     }
        // }
        // errs() << "===================================================\n";

        // if (!AnyChosen) {
        //     errs() << "ILP chose no packs; stopping.\n";
        //     break;
        // }

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
    } 
    else {
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
