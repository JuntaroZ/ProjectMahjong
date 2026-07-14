#include "YakuRules.h"

namespace mahjong::internal {

void HaiteiRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isHaitei) Add(yaku, HaiteiName, 1);
}

} // namespace mahjong::internal
