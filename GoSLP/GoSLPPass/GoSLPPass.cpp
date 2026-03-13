#include "CandidatePacks.hpp"
#include "Emit.hpp"
#include "ILP.hpp"
#include "PermuteDP.hpp"
#include "Reduction.hpp"
#include "ShuffleCost.hpp"
#include "VecGraph.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

using namespace llvm;

namespace {

static double toDouble(InstructionCost C) {
  if (!C.isValid())
    return 0.0;
  return static_cast<double>(C.getValue());
}

static double estimatePackConstructionCost(Type *LaneTy, unsigned Width,
                                           TargetTransformInfo &TTI) {
  if (!LaneTy || Width == 0)
    return 0.0;

  if (auto *SubVecTy = dyn_cast<FixedVectorType>(LaneTy)) {
    unsigned SubW = SubVecTy->getNumElements();
    auto *WideTy =
        FixedVectorType::get(SubVecTy->getElementType(), SubW * Width);

    double Cost = 0.0;
    for (unsigned I = 0; I < Width; ++I) {
      Cost += toDouble(TTI.getShuffleCost(TargetTransformInfo::SK_InsertSubvector,
                                          WideTy, WideTy, {},
                                          TargetTransformInfo::TCK_RecipThroughput,
                                          static_cast<int>(I * SubW),
                                          SubVecTy));
    }
    return Cost;
  }

  auto *VecTy = FixedVectorType::get(LaneTy, Width);
  double Cost = 0.0;
  for (unsigned I = 0; I < Width; ++I) {
    Cost += toDouble(TTI.getVectorInstrCost(
        Instruction::InsertElement, VecTy,
        TargetTransformInfo::TCK_RecipThroughput, I, nullptr, nullptr));
  }
  return Cost;
}

static double estimateVecSavings(const Instruction *I, unsigned Width,
                                 TargetTransformInfo &TTI,
                                 const DataLayout &DL) {
  if (!I || Width == 0)
    return 0.0;

  auto CostKind = TargetTransformInfo::TCK_RecipThroughput;

  if (auto *BO = dyn_cast<BinaryOperator>(I)) {
    Type *Ty = BO->getType();
    if (!Ty)
      return 0.0;

    double ScalarTotal = 0.0;
    double VecCost = 0.0;

    if (auto *SubVecTy = dyn_cast<FixedVectorType>(Ty)) {
      unsigned SubW = SubVecTy->getNumElements();
      auto *WideTy =
          FixedVectorType::get(SubVecTy->getElementType(), SubW * Width);
      ScalarTotal = toDouble(TTI.getArithmeticInstrCost(BO->getOpcode(), Ty,
                                                        CostKind)) *
                    Width;
      VecCost = toDouble(
          TTI.getArithmeticInstrCost(BO->getOpcode(), WideTy, CostKind));
    } else {
      auto *WideTy = FixedVectorType::get(Ty, Width);
      ScalarTotal =
          toDouble(TTI.getArithmeticInstrCost(BO->getOpcode(), Ty, CostKind)) *
          Width;
      VecCost =
          toDouble(TTI.getArithmeticInstrCost(BO->getOpcode(), WideTy, CostKind));
    }

    return VecCost - ScalarTotal;
  }

  auto estimateMem = [&](unsigned Opcode, Type *Ty, Align AlignV) {
    if (!Ty)
      return 0.0;

    double ScalarTotal = 0.0;
    double VecCost = 0.0;

    if (auto *SubVecTy = dyn_cast<FixedVectorType>(Ty)) {
      unsigned SubW = SubVecTy->getNumElements();
      auto *WideTy =
          FixedVectorType::get(SubVecTy->getElementType(), SubW * Width);
      ScalarTotal = toDouble(TTI.getMemoryOpCost(Opcode, Ty, AlignV, 0,
                                                 CostKind)) *
                    Width;
      VecCost =
          toDouble(TTI.getMemoryOpCost(Opcode, WideTy, AlignV, 0, CostKind));
    } else {
      auto *WideTy = FixedVectorType::get(Ty, Width);
      ScalarTotal =
          toDouble(TTI.getMemoryOpCost(Opcode, Ty, AlignV, 0, CostKind)) * Width;
      VecCost =
          toDouble(TTI.getMemoryOpCost(Opcode, WideTy, AlignV, 0, CostKind));
    }

    return VecCost - ScalarTotal;
  };

  if (auto *LI = dyn_cast<LoadInst>(I)) {
    return estimateMem(Instruction::Load, LI->getType(), LI->getAlign());
  }

  if (auto *SI = dyn_cast<StoreInst>(I)) {
    return estimateMem(Instruction::Store, SI->getValueOperand()->getType(),
                       SI->getAlign());
  }

  // Unsupported operation type for paper-parity objective terms.
  return 0.0;
}

static ILPModel buildILPModel(const CandidatePairs &C, ShuffleCost &SC,
                              TargetTransformInfo &TTI,
                              const DataLayout &DL) {
  ILPModel Model;
  const size_t N = C.Packs.size();
  Model.VecSavings.assign(N, 0.0);
  Model.PackCost.assign(N, 0.0);
  Model.LaneExtractCost.assign(N, {});

  for (size_t I = 0; I < N; ++I) {
    Node NNode;
    NNode.PackIdx = static_cast<int>(I);
    NNode.pack = C.Packs[I];

    Model.PackCost[I] = toDouble(SC.getPackCost(NNode));
    Model.VecSavings[I] = estimateVecSavings(
        C.Packs[I].empty() ? nullptr : C.Packs[I].front(),
        static_cast<unsigned>(C.Packs[I].size()), TTI, DL);

    auto &LaneCosts = Model.LaneExtractCost[I];
    LaneCosts.resize(C.Packs[I].size(), 0.0);
    for (unsigned Lane = 0; Lane < C.Packs[I].size(); ++Lane) {
      LaneCosts[Lane] = toDouble(SC.getExtractLaneCost(C.Packs[I], Lane));
    }
  }

  Model.NonVecPackCost.assign(C.NonVecPacks.size(), 0.0);
  for (size_t NV = 0; NV < C.NonVecPacks.size(); ++NV) {
    const auto &Key = C.NonVecPacks[NV];
    if (Key.Lanes.empty())
      continue;
    Type *LaneTy = Key.Lanes.front()->getType();
    Model.NonVecPackCost[NV] = estimatePackConstructionCost(
        LaneTy, static_cast<unsigned>(Key.Lanes.size()), TTI);
  }

  return Model;
}

class GoSLPPass : public PassInfoMixin<GoSLPPass> {
public:
  bool specific_function = false;
  std::string target_function;
  bool debug_flag = false;

  GoSLPPass() = default;
  explicit GoSLPPass(std::string FnName)
      : specific_function(true), target_function(std::move(FnName)) {}

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

} // namespace

PreservedAnalyses GoSLPPass::run(Function &F, FunctionAnalysisManager &FAM) {
  if (specific_function && !F.getName().contains(target_function))
    return PreservedAnalyses::all();

  AAResults &AA = FAM.getResult<AAManager>(F);
  auto &MSSAAnalysis = FAM.getResult<MemorySSAAnalysis>(F);
  MemorySSA &MSSA = MSSAAnalysis.getMSSA();
  TargetTransformInfo &TTI = FAM.getResult<TargetIRAnalysis>(F);
  const DataLayout &DL = F.getParent()->getDataLayout();

  errs() << "\n========== GoSLP on function " << F.getName()
         << " ==========\n";

  bool Changed = false;
  CandidatePairs C = collectCandidatePairs(F, AA, MSSA, debug_flag);
  if (debug_flag) {
    printCandidatePairs(C);
  } else {
    errs() << "Candidate packs: " << C.Packs.size()
           << ", vec-vec edges: " << C.VecVecUses.size()
           << ", non-vec packs: " << C.NonVecPacks.size() << "\n";
  }

  if (!C.Packs.empty()) {
    VecGraph G = buildVectorGraph(C);
    ShuffleCost SC = createShuffleCostCalculator(F, TTI, C);
    ILPModel Model = buildILPModel(C, SC, TTI, DL);

    if (debug_flag) {
      errs() << "==================== Vec Savings ====================\n";
      const size_t PrintLimit = 64;
      for (size_t I = 0; I < Model.VecSavings.size() && I < PrintLimit; ++I) {
        errs() << formatv("  Pack {0,3} : {1,8:F2}\n", I, Model.VecSavings[I]);
      }
      if (Model.VecSavings.size() > PrintLimit)
        errs() << "  ... (" << (Model.VecSavings.size() - PrintLimit)
               << " more entries elided)\n";
      errs() << "====================================================\n";
    }

    double TimeLimitSeconds = debug_flag
                                  ? 15.0
                                  : std::max(0.5, std::min(4.0, 0.05 * C.Packs.size()));
    std::vector<bool> Chosen = solveILP(C, Model, TimeLimitSeconds);
    bool AnyChosen = llvm::any_of(Chosen, [](bool V) { return V; });

    if (AnyChosen) {
      Perms LanePerm = choosePermutationsDP(G, Chosen, SC);
      Changed |= emit(F, C, Chosen, LanePerm, debug_flag);
    } else {
      errs() << "ILP chose no packs.\n";
    }
  } else {
    errs() << "No candidate packs for standard SLP.\n";
  }

  Changed |= runReductionAwareGoSLP(F, TTI, DL, debug_flag);

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GoSLPPass", "1.0",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement> Pipeline) {
                  if (Name != "GoSLPPass")
                    return false;

                  bool HasFilter = false;
                  std::string FnName;
                  bool DebugFlag = false;

                  for (auto &Elem : Pipeline) {
                    auto Parts = Elem.Name.split(':');

                    if (Parts.first == "func" && !Parts.second.empty()) {
                      FnName = Parts.second.str();
                      HasFilter = true;
                      continue;
                    }

                    if (Parts.first == "o3flag") {
                      DebugFlag = Parts.second.empty() || Parts.second == "true";
                    }
                  }

                  if (HasFilter) {
                    GoSLPPass P(FnName);
                    P.debug_flag = DebugFlag;
                    FPM.addPass(std::move(P));
                  } else {
                    GoSLPPass P;
                    P.debug_flag = DebugFlag;
                    FPM.addPass(std::move(P));
                  }

                  return true;
                });
          }};
}
