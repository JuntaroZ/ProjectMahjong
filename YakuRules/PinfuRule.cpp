#include "YakuRules.h"

namespace mahjong::internal {
namespace {

int TileIndex(const Tile& tile)
{
    switch (tile.suit) {
    case Suit::Man:
        return tile.rank - 1;
    case Suit::Pin:
        return 9 + tile.rank - 1;
    case Suit::Sou:
        return 18 + tile.rank - 1;
    case Suit::Honor:
        return 27 + tile.rank - 1;
    }

    return -1;
}

bool IsTwoSidedWait(const Decomposition& hand, const Tile& winningTile)
{
    const int winningTileIndex = TileIndex(winningTile);
    if (winningTileIndex == hand.pairTile) {
        return false;
    }

    for (const Meld& meld : hand.melds) {
        if (meld.type != Meld::Type::Sequence) {
            continue;
        }

        const int sequenceStart = meld.tile;
        const int sequenceStartRank = sequenceStart % 9 + 1;
        if (winningTileIndex == sequenceStart) {
            return sequenceStartRank != 7;
        }
        if (winningTileIndex == sequenceStart + 2) {
            return sequenceStartRank != 1;
        }
    }

    return false;
}

} // namespace

void PinfuRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || !context.isClosed) return;
    if (IsDragon(hand->pairTile) || IsWindTile(hand->pairTile, context.seatWind) || IsWindTile(hand->pairTile, context.roundWind)) {
        return;
    }
    if (AllMeldsAre(*hand, Meld::Type::Sequence) && IsTwoSidedWait(*hand, context.winningTile)) {
        Add(yaku, PinfuName, 1);
    }
}

} // namespace mahjong::internal
