#include "CandidatePacks.cpp"

struct Node {
    int PackIdx;
    std::vector<const Instruction *> pack; // not necessary because I can just use PackIdx but will help with printing
    std::vector<int> Defs; // list of PackIdx's this pack depends on
    std::vector<int> Uses; //same but for uses
};


struct VecGraph { 
    std::vector<Node> items; // each item will point to all its respective defs and uses
};


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
                        for (int childPack : It->second) {
                            if (childPack != id) {
                                graph.items[id].Uses.push_back(childPack);
                                graph.items[childPack].Defs.push_back(id);
                            }
                        }
                    }
                }
            }
        }
    }

    return graph;
}
