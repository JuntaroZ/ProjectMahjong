#include "YakuRules.h"

namespace mahjong::internal {

void RiichiRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isRiichi && context.isClosed) Add(yaku, RiichiName, 1);
}

} // namespace mahjong::internal
