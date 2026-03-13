#pragma once

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Function.h"

using namespace llvm;

bool runReductionAwareGoSLP(Function &F, TargetTransformInfo &TTI,
                            const DataLayout &DL, bool Debug);
