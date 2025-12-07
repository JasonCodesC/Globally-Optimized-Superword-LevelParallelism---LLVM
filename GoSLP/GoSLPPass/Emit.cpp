//Emit.cpp
#include "Emit.hpp"

bool good_mem_ops(const std::vector<const Instruction *> &lanes, const DataLayout &DL,
    Type *elem_ty, std::vector<int> &mem_index) {

    int width = lanes.size();
    mem_index.assign(width, 0);
    if (!elem_ty->isSized()) {
        return false;
    }

    int elem_size = DL.getTypeStoreSize(elem_ty);
    if (elem_size == 0) {
        return false;
    }

    std::vector<const Value *> bases(width, nullptr);
    std::vector<int> offs(width, 0);
    for (int i = 0; i < width; ++i) {
        const Instruction *inst = lanes[i];
        const Value *base = nullptr;
        int64_t byte_offset = 0;
        if (!getAddrBaseAndOffset(inst, DL, base, byte_offset)) {
            return false;
        }
        bases[i] = base;
        offs[i] = byte_offset;
    }


    int off_min = offs[0];
    for (int i = 1; i < width; ++i) {
        if (offs[i] < off_min)
            off_min = offs[i];
    }

    std::vector<int> pos(width, -1);
    std::vector<bool> used(width, false);
    for (int i = 0; i < width; ++i) {
        int diff = offs[i] - off_min;
        if (diff < 0) {
            return false;
        }
        if (diff % elem_size != 0) {
            return false;
        }
        int idx = (diff / elem_size);
        if (idx < 0 || idx >= width) {
            return false;
        }
        if (used[idx]) {
            return false;
        }
        used[idx] = true;
        pos[i] = idx;
    }

    for (int i = 0; i < width; ++i) {
        if (!used[i]) {
            return false;
        }
    }
    for (int i = 0; i < width; ++i) {
        mem_index[i] = pos[i];
    }

    return true;
}

bool iterativeLoadStorePack(const std::vector<const Instruction *> &lanes_copy, Type *val_ty, bool is_load, 
        const DataLayout &DL, LLVMContext &ctx, IRBuilder<> &builder, std::vector<Instruction *> &to_erase) {
            
    int width = lanes_copy.size();
    if (width == 0) {
        return false;
    }
    auto *vec_ty_in_lane = dyn_cast<FixedVectorType>(val_ty);
    if (!vec_ty_in_lane) {
        return false;
    }

    Type *scalar_ty = vec_ty_in_lane->getElementType();
    int sub_width = vec_ty_in_lane->getNumElements();
    int total_width = width * sub_width;
    std::vector<int> mem_index;
    if (!good_mem_ops(lanes_copy, DL, val_ty, mem_index)) {
        return false;
    }

    int base_lane = -1;
    for (int lane = 0; lane < width; ++lane) {
        if (mem_index[lane] == 0u) {
            base_lane = (int)lane;
            break;
        }
    }
    if (base_lane < 0) {
        return false;
    }

    auto *wide_vec_ty = FixedVectorType::get(scalar_ty, total_width);
    if (is_load) {
        auto *base_load = cast<LoadInst>(const_cast<Instruction *>(lanes_copy[base_lane]));
        Value *base_ptr = base_load->getPointerOperand();
        auto *ptr_ty = cast<PointerType>(base_ptr->getType());
        int addr_space = ptr_ty->getAddressSpace();
        auto *wide_ptr_ty = PointerType::get(wide_vec_ty, addr_space);
        Value *wide_ptr = builder.CreateBitCast(base_ptr, wide_ptr_ty, "goslp.ld.ptr.wide");
        LoadInst *wide_load = builder.CreateLoad(wide_vec_ty, wide_ptr, "goslp.vload.wide");
        wide_load->setAlignment(base_load->getAlign());

        for (int lane = 0; lane < width; ++lane) {
            Instruction *old_inst = const_cast<Instruction *>(lanes_copy[lane]);
            int block_idx = mem_index[lane];
            int start_scalar = block_idx * sub_width;

            std::vector<Constant *> mask_elems;
            mask_elems.reserve(sub_width);
            for (int j = 0; j < sub_width; ++j) {
                int wide_index = start_scalar + j;
                mask_elems.push_back(ConstantInt::get(Type::getInt32Ty(ctx), wide_index));
            }

            Constant *mask = ConstantVector::get(mask_elems);
            Value *undef_vec = UndefValue::get(wide_vec_ty);
            Value *replacement = builder.CreateShuffleVector(wide_load, undef_vec, mask, "goslp.ld.wide.subvec");

            old_inst->replaceAllUsesWith(replacement);
            to_erase.push_back(old_inst);
        }
        return true;
    } 
    else {
        auto *base_store = cast<StoreInst>(const_cast<Instruction *>(lanes_copy[base_lane]));
        Value *base_ptr = base_store->getPointerOperand();
        auto *ptr_ty = cast<PointerType>(base_ptr->getType());
        int addr_space = ptr_ty->getAddressSpace();
        auto *wide_ptr_ty = PointerType::get(wide_vec_ty, addr_space);
        Value *wide_val = UndefValue::get(wide_vec_ty);

        for (int lane = 0; lane < width; ++lane) {
            auto *st = cast<StoreInst>(const_cast<Instruction *>(lanes_copy[lane]));
            Value *lane_val = st->getValueOperand(); 

            int block_idx = mem_index[lane];
            int start_scalar = block_idx * sub_width;
            for (int j = 0; j < sub_width; ++j) {
                Value *idx_val = builder.getInt32(j);
                Value *elem = builder.CreateExtractElement(lane_val, idx_val, "goslp.st.wide.elem");
                int wide_index = start_scalar + j;
                wide_val = builder.CreateInsertElement(wide_val, elem, builder.getInt32(wide_index), "goslp.st.wide.ins");
            }
        }

        Value *wide_ptr = builder.CreateBitCast(base_ptr, wide_ptr_ty, "goslp.st.ptr.wide");
        StoreInst *wide_store = builder.CreateStore(wide_val, wide_ptr);
        wide_store->setAlignment(base_store->getAlign());

        for (int lane = 0; lane < width; ++lane) {
            Instruction *st_inst = const_cast<Instruction *>(lanes_copy[lane]);
            to_erase.push_back(st_inst);
        }

        return true;
    }
}

bool emit(Function &F, CandidatePairs &C, const std::vector<bool> &Chosen, const Perms &LanePerm) {
    bool changed = false;
    std::vector<Instruction *> to_erase;

    if (C.Packs.empty() || Chosen.size() < C.Packs.size()) {
        return false;
    }

    Module *M = F.getParent();
    if (!M) {
        return false;
    }
    const DataLayout &DL = M->getDataLayout();
    LLVMContext &ctx = F.getContext();

    // Emit load/store packs first, then arithmetic, to keep dominance sane.
    std::vector<int> worklist;
    for (int i = 0; i < static_cast<int>(C.Packs.size()); ++i) {
        if (Chosen[i])
            worklist.push_back(i);
    }
    auto isLoadOrStorePack = [](const std::vector<const Instruction *> &lanes) {
        return llvm::all_of(lanes, [](const Instruction *I) { return isa<LoadInst>(I); }) ||
               llvm::all_of(lanes, [](const Instruction *I) { return isa<StoreInst>(I); });
    };
    std::stable_partition(worklist.begin(), worklist.end(), [&](int idx) {
        return isLoadOrStorePack(C.Packs[idx]);
    });

    for (int idx : worklist) {
        const auto &lanes = C.Packs[idx];
        if (lanes.empty())
            continue;

        bool all_binop = true;
        bool all_load  = true;
        bool all_store = true;
        for (const Instruction *inst : lanes) {
            const Instruction *ni = const_cast<Instruction *>(inst);
            if (!isa<BinaryOperator>(ni))
                all_binop = false;
            if (!isa<LoadInst>(ni))
                all_load = false;
            if (!isa<StoreInst>(ni))
                all_store = false;
        }

        if (!all_binop && !all_load && !all_store) {
            continue;
        }

        std::vector<const Instruction *> lanes_copy = lanes;
        auto perm = LanePerm.find(idx);
        if (perm != LanePerm.end() && perm->second.size() == lanes_copy.size()) {
            const Permutation &P = perm->second;
            std::vector<const Instruction *> permuted(lanes_copy.size());
            for (size_t j = 0; j < lanes_copy.size(); ++j) {
                int src = P[j];
                if (src < static_cast<int>(lanes_copy.size())) {
                    permuted[j] = lanes_copy[src];
                } 
                else {
                    permuted[j] = lanes_copy[j];
                }
            }
            lanes_copy.swap(permuted);
        }
        
        Instruction *insert_pt = const_cast<Instruction *>(lanes_copy[0]);
        for (const Instruction *curr : lanes_copy) {
            Instruction *inst = const_cast<Instruction *>(curr);
            if (inst->getParent() == insert_pt->getParent() && inst->comesBefore(insert_pt)) {
                insert_pt = inst;
            }
        }

        Instruction *insertBefore = insert_pt;

        IRBuilder<> builder(insertBefore);
        int width = lanes_copy.size();
        if (all_binop) {
            auto *bin_op = cast<BinaryOperator>(const_cast<Instruction *>(lanes_copy[0]));

            Type *result_ty = bin_op->getType();
            Type *scalar_ty = nullptr;
            int sub_width = 1;

            if (auto *vec_ty = dyn_cast<FixedVectorType>(result_ty)) {
                scalar_ty = vec_ty->getElementType();
                sub_width = vec_ty->getNumElements();
            } 
            else {
                scalar_ty = result_ty;
                sub_width = 1;
            }

            int total_width = width * sub_width;
            auto *wide_vec_ty = FixedVectorType::get(scalar_ty, total_width);
            Value *vec_op0 = UndefValue::get(wide_vec_ty);
            Value *vec_op1 = UndefValue::get(wide_vec_ty);

            for (int lane = 0; lane < width; ++lane) {
                auto *lane_op = cast<BinaryOperator>(const_cast<Instruction *>(lanes_copy[lane]));
                Value *op0 = lane_op->getOperand(0);
                Value *op1 = lane_op->getOperand(1);
            
                if (sub_width == 1) {
                    int wide_index = lane;
                    vec_op0 = builder.CreateInsertElement(
                        vec_op0, op0, ConstantInt::get(Type::getInt32Ty(ctx), wide_index), "goslp.ins0");
                    vec_op1 = builder.CreateInsertElement(
                        vec_op1, op1, ConstantInt::get(Type::getInt32Ty(ctx), wide_index), "goslp.ins1");
                } 
                else {
                    for (int j = 0; j < sub_width; ++j) {
                        Value *idx_val = builder.getInt32(j);
                        Value *elem0 = builder.CreateExtractElement(op0, idx_val, "goslp.op0.elem");
                        Value *elem1 = builder.CreateExtractElement(op1, idx_val, "goslp.op1.elem");
                        int wide_index = lane * sub_width + j;

                        vec_op0 = builder.CreateInsertElement(vec_op0, elem0, ConstantInt::get(Type::getInt32Ty(ctx), wide_index), "goslp.ins0");
                        vec_op1 = builder.CreateInsertElement(vec_op1, elem1, ConstantInt::get(Type::getInt32Ty(ctx), wide_index), "goslp.ins1");
                    }
                }
            }

            Instruction *vec_bin_op = cast<Instruction>(
                builder.CreateBinOp(bin_op->getOpcode(), vec_op0, vec_op1, "goslp.vec"));

            // Insert scalar extracts at the original instruction sites to
            // guarantee dominance over all original uses.
            for (int lane = 0; lane < width; ++lane) {
                Instruction *old_inst = const_cast<Instruction *>(lanes_copy[lane]);
                IRBuilder<> extBuilder(old_inst);

                Value *laneVal = nullptr;
                if (sub_width == 1) {
                    int wide_index = lane;
                    laneVal = extBuilder.CreateExtractElement(
                        vec_bin_op, ConstantInt::get(Type::getInt32Ty(ctx), wide_index), "goslp.ext");
                } else {
                    SmallVector<Constant *, 8> mask_elems;
                    mask_elems.reserve(sub_width);
                    for (int j = 0; j < sub_width; ++j) {
                        int wide_index = lane * sub_width + j;
                        mask_elems.push_back(ConstantInt::get(Type::getInt32Ty(ctx), wide_index));
                    }
                    Constant *mask = ConstantVector::get(mask_elems);
                    Value *undef_vec = UndefValue::get(wide_vec_ty);
                    laneVal = extBuilder.CreateShuffleVector(vec_bin_op, undef_vec, mask, "goslp.subvec");
                }

                old_inst->replaceAllUsesWith(laneVal);
                to_erase.push_back(old_inst);
            }
            changed = true;
            continue;
        }

        if (all_load) {
            insert_pt = const_cast<Instruction *>(lanes_copy[0]);
            for (const Instruction *curr : lanes_copy) {
                Instruction *inst = const_cast<Instruction *>(curr);
                if (inst->getParent() == insert_pt->getParent() && inst->comesBefore(insert_pt)) {
                    insert_pt = inst;
                }
            }
            IRBuilder<> builder(insert_pt);
            auto *first_load = cast<LoadInst>(const_cast<Instruction *>(lanes_copy[0]));
            Type *val_ty = first_load->getType();


            if (val_ty->isVectorTy()) {
                if (iterativeLoadStorePack(lanes_copy, val_ty, true, DL, ctx, builder, to_erase)) {
                    changed = true;
                    continue;
                }
            }

            std::vector<int> mem_index;
            if (!good_mem_ops(lanes_copy, DL, val_ty, mem_index)) {
                continue;
            }

            int base_lane = -1;
            for (int lane = 0; lane < width; ++lane) {
                if (mem_index[lane] == 0) {
                    base_lane = lane;
                    break;
                }
            }
            if (base_lane < 0) {
                continue;
            }

            auto *base_load = cast<LoadInst>(const_cast<Instruction *>(lanes_copy[base_lane]));
            Value *base_ptr = base_load->getPointerOperand();
            auto *ptr_ty = cast<PointerType>(base_ptr->getType());
            int addr_space = ptr_ty->getAddressSpace();
            auto *vec_ty = FixedVectorType::get(val_ty, width);
            auto *vec_ptr_ty = PointerType::get(vec_ty, addr_space);

            Value *vec_ptr = builder.CreateBitCast(base_ptr, vec_ptr_ty, "goslp.ld.ptr");
            LoadInst *vec_load = builder.CreateLoad(vec_ty, vec_ptr, "goslp.vload");
            vec_load->setAlignment(first_load->getAlign());

            for (int lane = 0; lane < width; ++lane) {
                int elem_idx = mem_index[lane];
                Instruction *ld_inst = const_cast<Instruction *>(lanes_copy[lane]);
                Value *elem = builder.CreateExtractElement(vec_load, builder.getInt32(elem_idx), "goslp.ld.ext");
                ld_inst->replaceAllUsesWith(elem);
                to_erase.push_back(ld_inst);
            }

            changed = true;
            continue;
        }

        if (all_store) {
            auto *first_store = cast<StoreInst>(const_cast<Instruction *>(lanes_copy[0]));
            Type *val_ty = first_store->getValueOperand()->getType();

            if (val_ty->isVectorTy()) {
                if (iterativeLoadStorePack(lanes_copy, val_ty, false, DL, ctx, builder, to_erase)) {
                    changed = true;
                    continue;
                }
            }

            std::vector<int> mem_index;
            if (!good_mem_ops(lanes_copy, DL, val_ty, mem_index)) {
                continue;
            }

            int base_lane = -1;
            for (int lane = 0; lane < width; ++lane) {
                if (mem_index[lane] == 0u) {
                    base_lane = (int)lane;
                    break;
                }
            }
            if (base_lane < 0) {
                continue;
            }

            auto *base_store = cast<StoreInst>(const_cast<Instruction *>(lanes_copy[base_lane]));
            Value *base_ptr = base_store->getPointerOperand();
            auto *ptr_ty = cast<PointerType>(base_ptr->getType());
            int addr_space = ptr_ty->getAddressSpace();
            auto *vec_ty = FixedVectorType::get(val_ty, width);
            auto *vec_ptr_ty = PointerType::get(vec_ty, addr_space);

            Value *vec_val = UndefValue::get(vec_ty);
            for (int lane = 0; lane < width; ++lane) {
                int elem_idx = mem_index[lane];
                auto *st = cast<StoreInst>(const_cast<Instruction *>(lanes_copy[lane]));
                Value *v = st->getValueOperand();
                vec_val = builder.CreateInsertElement(vec_val, v, builder.getInt32(elem_idx), "goslp.st.ins");
            }

            Value *vec_ptr = builder.CreateBitCast(base_ptr, vec_ptr_ty, "goslp.st.ptr");
            StoreInst *vec_store = builder.CreateStore(vec_val, vec_ptr);
            vec_store->setAlignment(first_store->getAlign());

            for (int lane = 0; lane < width; ++lane) {
                Instruction *st_inst = const_cast<Instruction *>(lanes_copy[lane]);
                to_erase.push_back(st_inst);
            }

            changed = true;
            continue;
        }
    }

    for (Instruction *inst : to_erase) {
        if (!inst->use_empty()) {
        } else {
            inst->eraseFromParent();
        }
    }

    return changed;
}
