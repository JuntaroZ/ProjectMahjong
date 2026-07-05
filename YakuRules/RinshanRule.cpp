#include "YakuRules.h"

namespace mahjong::internal {

void RinshanRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isRinshan) Add(yaku, RinshanName, 1);
}

} // namespace mahjong::internal
