// VecGraph
#include "VecGraph.hpp"

VecGraph buildVectorGraph(CandidatePairs& C) {
    VecGraph graph;

    // add all nodes with no defs/uses at first
    for (int i = 0; i < (int)C.Packs.size(); i++) {
        Node N;
        N.PackIdx = i;
        N.pack = C.Packs[i];
        graph.items.push_back(std::move(N));
    }
    
    // add edges using use-def
    for (int id = 0; id < (int)graph.items.size(); id++) {
        for (const Instruction *I : graph.items[id].pack) {
            for (const User *U : I->users()) {
                if (auto *UI = dyn_cast<Instruction>(U)) {
                    auto It = C.InstToCandidates.find(UI);
                    if (It != C.InstToCandidates.end()) {
                        for (auto childPack : It->second) {
                            if (childPack.Index != id) {
                                graph.items[id].Uses.push_back(childPack.Index);
                                graph.items[childPack.Index].Defs.push_back(id);
                            }
                        }
                    }
                }
            }
        }
    }

    return graph;
}
