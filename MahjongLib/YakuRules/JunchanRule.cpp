#include "YakuRules.h"

namespace mahjong::internal {
namespace {

bool IsTerminalTile(int tile)
{
    return tile < 27 && (tile % 9 == 0 || tile % 9 == 8);
}

bool MeldHasTerminal(const Meld& meld)
{
    if (meld.type == Meld::Type::Triplet) {
        return IsTerminalTile(meld.tile);
    }
    return IsTerminalTile(meld.tile) || IsTerminalTile(meld.tile + 2);
}

} // namespace

void JunchanRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || !IsTerminalTile(hand->pairTile)) {
        return;
    }

    bool hasSequence = false;
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Sequence) {
            hasSequence = true;
        }
        if (!MeldHasTerminal(meld)) {
            return;
        }
    }

    if (hasSequence) {
        Add(yaku, JunchanName, context.isClosed ? 3 : 2);
    }
}

} // namespace mahjong::internal
