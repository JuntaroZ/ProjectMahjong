#include "YakuRules.h"

namespace mahjong::internal {

void ToitoiRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (hand != nullptr && AllMeldsAre(*hand, Meld::Type::Triplet)) {
        Add(yaku, ToitoiName, 2);
    }
}

} // namespace mahjong::internal
