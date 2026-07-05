#include "YakuRules.h"

namespace mahjong::internal {

void ChiitoitsuRule::Evaluate(const HandAnalysis& analysis, const Decomposition* hand, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr && analysis.sevenPairs) {
        Add(yaku, ChiitoitsuName, 2);
    }
}

} // namespace mahjong::internal
