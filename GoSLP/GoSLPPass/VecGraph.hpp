#pragma once
#include "CandidatePacks.hpp"

struct Node {
    int PackIdx;
    std::vector<const Instruction *> pack; // not necessary because I can just use PackIdx but will help with printing
    std::vector<int> Defs; // list of PackIdx's this pack depends on
    std::vector<int> Uses; //same but for uses
};


struct VecGraph { 
    std::vector<Node> items; // each item will point to all its respective defs and uses
};

VecGraph buildVectorGraph(CandidatePairs& C);