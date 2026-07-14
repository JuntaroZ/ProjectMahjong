#include "YakuRules.h"

namespace mahjong::internal {

void ChihoRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isChiho) AddYakuman(yaku, ChihoName);
}

} // namespace mahjong::internal
