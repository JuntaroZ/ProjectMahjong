#include "YakuRules.h"

namespace mahjong::internal {

void KokushiRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (analysis.kokushi) AddYakuman(yaku, KokushiName);
}

} // namespace mahjong::internal
