#include "YakuRules.h"

namespace mahjong::internal {

void HonitsuRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (IsFlushFamily(analysis, true) && !IsFlushFamily(analysis, false)) {
        Add(yaku, HonitsuName, context.isClosed ? 3 : 2);
    }
}

} // namespace mahjong::internal
