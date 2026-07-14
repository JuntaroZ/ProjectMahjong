#include "YakuRules.h"

#include <algorithm>

namespace mahjong::internal {

void SanshokuDoukouRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr) {
        return;
    }

    for (int rank = 0; rank < 9; ++rank) {
        const bool man = std::any_of(hand->melds.begin(), hand->melds.end(), [rank](const Meld& meld) {
            return meld.type == Meld::Type::Triplet && meld.tile == rank;
        });
        const bool pin = std::any_of(hand->melds.begin(), hand->melds.end(), [rank](const Meld& meld) {
            return meld.type == Meld::Type::Triplet && meld.tile == 9 + rank;
        });
        const bool sou = std::any_of(hand->melds.begin(), hand->melds.end(), [rank](const Meld& meld) {
            return meld.type == Meld::Type::Triplet && meld.tile == 18 + rank;
        });

        if (man && pin && sou) {
            Add(yaku, SanshokuDoukouName, 2);
            return;
        }
    }
}

} // namespace mahjong::internal
