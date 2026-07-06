#include "YakuRules.h"

#include <array>

namespace mahjong::internal {

void RyanpeikouRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || !context.isClosed) {
        return;
    }

    std::array<int, 27> sequenceCounts{};
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Sequence) {
            ++sequenceCounts[meld.tile];
        }
    }

    int pairs = 0;
    for (int count : sequenceCounts) {
        if (count >= 2) {
            ++pairs;
        }
    }

    if (pairs >= 2) {
        Add(yaku, RyanpeikouName, 3);
    }
}

} // namespace mahjong::internal
