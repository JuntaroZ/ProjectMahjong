#include "YakuRules.h"

namespace mahjong::internal {
namespace {

bool IsWind(int tile)
{
    return tile >= 27 && tile <= 30;
}

} // namespace

void DaisuushiiRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext&, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr) {
        return;
    }

    int windTriplets = 0;
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Triplet && IsWind(meld.tile)) {
            ++windTriplets;
        }
    }

    if (windTriplets == 4) {
        AddYakuman(yaku, DaisuushiiName);
    }
}

} // namespace mahjong::internal
