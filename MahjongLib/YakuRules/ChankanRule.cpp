#include "YakuRules.h"

namespace mahjong::internal {

void ChankanRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isChankan) Add(yaku, ChankanName, 1);
}

} // namespace mahjong::internal
