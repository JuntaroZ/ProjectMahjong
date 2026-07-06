#include "YakuRules.h"

namespace mahjong::internal {

void HonroutouRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext&, std::vector<Yaku>& yaku) const
{
    for (int tile = 0; tile < TileKindCount; ++tile) {
        if (analysis.counts[tile] > 0 && !IsTerminalOrHonor(tile)) {
            return;
        }
    }

    Add(yaku, HonroutouName, 2);
}

} // namespace mahjong::internal
