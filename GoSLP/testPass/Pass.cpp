#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <iostream>

using namespace llvm;

namespace {

struct FunctionNamePass : public PassInfoMixin<FunctionNamePass> {

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {

    std::cout << "Hello function: " << F.getName().str() << std::endl;
    return PreservedAnalyses::all();
  }
};
} // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "FunctionNamePass", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "func-name") {
                    FPM.addPass(FunctionNamePass());
                    return true;
                  }
                  return false;
                });
          }};
}