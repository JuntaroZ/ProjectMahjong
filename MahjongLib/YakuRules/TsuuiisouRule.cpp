#include "YakuRules.h"

namespace mahjong::internal {

void TsuuiisouRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext&, std::vector<Yaku>& yaku) const
{
    for (int tile = 0; tile < TileKindCount; ++tile) {
        if (analysis.counts[tile] > 0 && !IsHonor(tile)) {
            return;
        }
    }

    AddYakuman(yaku, TsuuiisouName);
}

} // namespace mahjong::internal
