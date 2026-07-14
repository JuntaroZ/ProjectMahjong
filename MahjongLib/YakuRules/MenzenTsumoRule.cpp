#include "YakuRules.h"

namespace mahjong::internal {

void MenzenTsumoRule::Evaluate(const HandAnalysis&, const Decomposition*, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (context.isTsumo && context.isClosed) Add(yaku, MenzenTsumoName, 1);
}

} // namespace mahjong::internal
