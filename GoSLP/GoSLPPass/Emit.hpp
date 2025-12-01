#pragma once
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include <vector>
#include <cstdint>
#include "CandidatePacks.hpp"
#include "PermuteDP.hpp"

using namespace llvm;

bool good_mem_ops(const std::vector<const Instruction *> &lanes, const DataLayout &DL,
    Type *elem_ty, std::vector<int> &mem_index);
bool iterativeLoadStorePack(const std::vector<const Instruction *> &lanes_copy, Type *val_ty, bool is_load, 
        const DataLayout &DL, LLVMContext &ctx, IRBuilder<> &builder, std::vector<Instruction *> &to_erase);
bool emit(Function &F, CandidatePairs &C, const std::vector<bool> &Chosen, const Perms &LanePerm);