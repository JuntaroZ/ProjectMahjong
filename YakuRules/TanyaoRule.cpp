#include "YakuRules.h"

namespace mahjong::internal {

void TanyaoRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (!context.isClosed && !context.allowOpenTanyao) {
        return;
    }

    for (int tile = 0; tile < TileKindCount; ++tile) {
        if (analysis.counts[tile] > 0 && IsTerminalOrHonor(tile)) {
            return;
        }
    }
    Add(yaku, TanyaoName, 1);
}

} // namespace mahjong::internal
