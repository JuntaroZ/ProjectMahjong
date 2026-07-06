#include "YakuRules.h"

namespace mahjong::internal {
namespace {

bool IsGreenTile(int tile)
{
    return tile == 19 || tile == 20 || tile == 21 || tile == 23 || tile == 25 || tile == 32;
}

} // namespace

void RyuuiisouRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext&, std::vector<Yaku>& yaku) const
{
    for (int tile = 0; tile < TileKindCount; ++tile) {
        if (analysis.counts[tile] > 0 && !IsGreenTile(tile)) {
            return;
        }
    }

    AddYakuman(yaku, RyuuiisouName);
}

} // namespace mahjong::internal
