#include "YakuRules.h"

namespace mahjong::internal {

void ChinroutouRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext&, std::vector<Yaku>& yaku) const
{
    for (int tile = 0; tile < TileKindCount; ++tile) {
        if (analysis.counts[tile] > 0 && !IsTerminal(tile)) {
            return;
        }
    }

    AddYakuman(yaku, ChinroutouName);
}

} // namespace mahjong::internal
