#include "YakuRules.h"

namespace mahjong::internal {

void IppatsuRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isIppatsu && context.isRiichi) Add(yaku, IppatsuName, 1);
}

} // namespace mahjong::internal
