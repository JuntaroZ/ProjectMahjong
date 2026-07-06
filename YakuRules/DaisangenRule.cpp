#include "YakuRules.h"

namespace mahjong::internal {

void DaisangenRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr) {
        return;
    }

    int dragonTriplets = 0;
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Triplet && IsDragon(meld.tile)) {
            ++dragonTriplets;
        }
    }

    if (dragonTriplets == 3) {
        AddYakuman(yaku, DaisangenName);
    }
}

} // namespace mahjong::internal
