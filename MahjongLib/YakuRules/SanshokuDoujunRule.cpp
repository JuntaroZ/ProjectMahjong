#include "YakuRules.h"

namespace mahjong::internal {

void SanshokuDoujunRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr) return;

    for (int rank = 0; rank <= 6; ++rank) {
        if (HasSequence(*hand, rank) && HasSequence(*hand, 9 + rank) && HasSequence(*hand, 18 + rank)) {
            Add(yaku, SanshokuDoujunName, context.isClosed ? 2 : 1);
            return;
        }
    }
}

} // namespace mahjong::internal
