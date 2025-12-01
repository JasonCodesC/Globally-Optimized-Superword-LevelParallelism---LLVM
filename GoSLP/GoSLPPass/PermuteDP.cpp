// DP
#include "PermuteDP.hpp"

PermsList generatePerms(unsigned width){
    PermsList list;
    if (width == 0) return list;

    // identify permutation
    Permutation base(width);
    for (unsigned i = 0; i < width; i++){
        base[i] = i;
    }
    if (width <= 4){
        Permutation p = base;
        list.push_back(p);
        while (std::next_permutation(p.begin(), p.end())){
            list.push_back(p);
        }
    }
    else {
        list.push_back(base);
    }
    return list;
}
static std::vector<const Instruction *> applyPermutation(const std::vector<const Instruction *> &pack, const Permutation &p) {
    std::vector<const Instruction *> out(pack.size());
    for (size_t i = 0; i < pack.size(); ++i)
        out[i] = pack[p[i]];
    return out;
}


// DP implementation
Perms choosePermutationsDP(const VecGraph &G, ShuffleCost &SC) {
    Perms result;
    unsigned n = G.items.size();
    if (n==0) return result;

    // compute permutations for each pack
    std::vector<PermsList> total(n);
    for (int i = 0; i < n; i++){
        int width = G.items[i].pack.size();
        total[i] = generatePerms(width);
    }

    // DP table dp[i][x] where best cost if pack i uses permutation x
    std::vector<CostVec> dp(n);
    for(int i = 0; i < n; i++){
        dp[i] = CostVec(total[i].size(), InstructionCost(0));
    }

    //topological sort
    std::vector<int> order;
    std::vector<int> indeg(n,0);
    std::queue<int> q;
    for (int i = 0; i < n; i++){
        indeg[i] = G.items[i].Defs.size();
    }
    
    for(int i = 0; i < n; i++){
        if (indeg[i] == 0) q.push(i);
    }
    
    while(!q.empty()){
        int x = q.front();
        q.pop();
        order.push_back(x);

        for(int u : G.items[x].Uses){
            indeg[u]--;
            if (indeg[u] == 0) q.push(u);
        }
    }
    
    // bottom-up DP
    for (auto it = order.rbegin(); it != order.rend(); ++it){
        int v = *it;
        const Node &vv = G.items[v];
        const PermsList &vp = total[v];

        if (vv.Uses.empty()) continue;

        for (size_t x = 0; x < vp.size(); x++){
            InstructionCost num = 0;
            Node vtemp = vv;
            vtemp.pack = applyPermutation(vv.pack, vp[x]);
            for (int succ: vv.Uses){
                PermsList &sp = total[succ];
                CostVec &succDP = dp[succ];
                InstructionCost best = InstructionCost::getInvalid();

                for (size_t sj = 0; sj < sp.size(); sj++){
                    Node stemp = G.items[succ];
                    stemp.pack = applyPermutation(stemp.pack, sp[sj]);
                    InstructionCost edge = SC.getShuffleCost(vtemp, stemp);
                    InstructionCost cand = edge + succDP[sj];

                    if (!best.isValid() || cand < best){
                        best = cand;
                    }
                }

                if (!best.isValid()){
                    best = InstructionCost(0);
                }
                num += best;
            }
            dp[v][x] = num;
        }
    }

    for (int i = 0; i < n; i++){
        const CostVec &costs = dp[i];
        const PermsList &li = total[i];
        size_t idx = 0;
        for (size_t j = 1; j < costs.size(); j++){
            if (costs[j] < costs[idx]){
                idx = j;
            }
        }
        result[i] = li[idx];
    }
    return result;

}