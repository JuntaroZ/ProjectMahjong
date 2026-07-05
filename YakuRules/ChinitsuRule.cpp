#include "YakuRules.h"

namespace mahjong::internal {

void ChinitsuRule::Evaluate(const HandAnalysis& analysis, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (IsFlushFamily(analysis, false)) {
        Add(yaku, ChinitsuName, context.isClosed ? 6 : 5);
    }
}

} // namespace mahjong::internal
