#include "YakuRules.h"

namespace mahjong::internal {
namespace {

bool IsWind(int tile)
{
    return tile >= 27 && tile <= 30;
}

} // namespace

void ShousuushiiRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || !IsWind(hand->pairTile)) {
        return;
    }

    int windTriplets = 0;
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Triplet && IsWind(meld.tile)) {
            ++windTriplets;
        }
    }

    if (windTriplets == 3) {
        AddYakuman(yaku, ShousuushiiName);
    }
}

} // namespace mahjong::internal
