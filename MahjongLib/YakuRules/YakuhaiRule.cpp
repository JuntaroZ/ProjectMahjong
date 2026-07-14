#include "YakuRules.h"

namespace mahjong::internal {

void YakuhaiRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr) return;

    for (const Meld& meld : hand->melds) {
        if (meld.type != Meld::Type::Triplet) continue;
        if (IsDragon(meld.tile) || IsWindTile(meld.tile, context.seatWind) || IsWindTile(meld.tile, context.roundWind)) {
            Add(yaku, YakuhaiName, 1);
        }
    }
}

} // namespace mahjong::internal
