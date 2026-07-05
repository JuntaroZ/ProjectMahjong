#include "YakuRules.h"

namespace mahjong::internal {

void IttsuRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr) return;

    for (int suit = 0; suit < 3; ++suit) {
        const int base = suit * 9;
        if (HasSequence(*hand, base) && HasSequence(*hand, base + 3) && HasSequence(*hand, base + 6)) {
            Add(yaku, IttsuName, context.isClosed ? 2 : 1);
            return;
        }
    }
}

} // namespace mahjong::internal
