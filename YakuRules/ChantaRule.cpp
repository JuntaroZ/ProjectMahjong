#include "YakuRules.h"

namespace mahjong::internal {
namespace {

bool IsHonorTile(int tile)
{
    return tile >= 27;
}

bool IsTerminalTile(int tile)
{
    return tile < 27 && (tile % 9 == 0 || tile % 9 == 8);
}

bool IsTerminalOrHonorTile(int tile)
{
    return IsTerminalTile(tile) || IsHonorTile(tile);
}

bool MeldHasTerminalOrHonor(const Meld& meld)
{
    if (meld.type == Meld::Type::Triplet) {
        return IsTerminalOrHonorTile(meld.tile);
    }
    return IsTerminalTile(meld.tile) || IsTerminalTile(meld.tile + 2);
}

} // namespace

void ChantaRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || !IsTerminalOrHonorTile(hand->pairTile)) {
        return;
    }

    bool hasSequence = false;
    bool hasHonor = IsHonorTile(hand->pairTile);
    for (const Meld& meld : hand->melds) {
        if (meld.type == Meld::Type::Sequence) {
            hasSequence = true;
        }
        if (meld.type == Meld::Type::Triplet && IsHonorTile(meld.tile)) {
            hasHonor = true;
        }
        if (!MeldHasTerminalOrHonor(meld)) {
            return;
        }
    }

    if (hasSequence && hasHonor) {
        Add(yaku, ChantaName, context.isClosed ? 2 : 1);
    }
}

} // namespace mahjong::internal
