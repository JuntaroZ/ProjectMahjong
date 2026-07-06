#include "YakuRules.h"

namespace mahjong::internal {

void ShousangenRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || !IsDragon(hand->pairTile)) {
        return;
    }

    int dragonTriplets = 0;
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Triplet && IsDragon(meld.tile)) {
            ++dragonTriplets;
        }
    }

    if (dragonTriplets == 2) {
        Add(yaku, ShousangenName, 2);
    }
}

} // namespace mahjong::internal
