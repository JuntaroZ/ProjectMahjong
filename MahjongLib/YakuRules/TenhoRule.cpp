#include "YakuRules.h"

namespace mahjong::internal {

void TenhoRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isTenho) AddYakuman(yaku, TenhoName);
}

} // namespace mahjong::internal
