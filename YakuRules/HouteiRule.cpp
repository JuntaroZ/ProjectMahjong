#include "YakuRules.h"

namespace mahjong::internal {

void HouteiRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isHoutei) Add(yaku, HouteiName, 1);
}

} // namespace mahjong::internal
