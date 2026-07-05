#include "YakuRules.h"

#include <algorithm>
#include <array>

namespace mahjong::internal {

void IipeikouRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || !context.isClosed) return;

    std::array<int, TileKindCount> sequenceCounts{};
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Sequence) {
            ++sequenceCounts[meld.tile];
        }
    }

    if (std::any_of(sequenceCounts.begin(), sequenceCounts.end(), [](int count) { return count >= 2; })) {
        Add(yaku, IipeikouName, 1);
    }
}

} // namespace mahjong::internal
