#include "YakuRules.h"

namespace mahjong::internal {

void ChuurenPoutouRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (!context.isClosed || !IsFlushFamily(analysis, false)) {
        return;
    }

    int suitBase = -1;
    for (int tile = 0; tile < 27; ++tile) {
        if (analysis.counts[tile] > 0) {
            suitBase = (tile / 9) * 9;
            break;
        }
    }
    if (suitBase < 0) {
        return;
    }

    if (analysis.counts[suitBase] < 3 || analysis.counts[suitBase + 8] < 3) {
        return;
    }
    for (int offset = 1; offset <= 7; ++offset) {
        if (analysis.counts[suitBase + offset] < 1) {
            return;
        }
    }

    AddYakuman(yaku, ChuurenPoutouName);
}

} // namespace mahjong::internal
