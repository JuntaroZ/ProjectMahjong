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

bool HasTriplet(const Decomposition& hand, int tile)
{
    for (const Meld& meld : hand.melds) {
        if (!meld.open && meld.type == Meld::Type::Triplet && meld.tile == tile) {
            return true;
        }
    }
    return false;
}

int CountConcealedTriplets(const Decomposition& hand)
{
    int count = 0;
    for (const Meld& meld : hand.melds) {
        if (!meld.open && meld.type == Meld::Type::Triplet) {
            ++count;
        }
    }
    return count;
}

} // namespace

void SuuankouRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr || CountConcealedTriplets(*hand) < 4) {
        return;
    }

    const int winningTile = TileIndex(context.winningTile);
    if (!context.isTsumo && HasTriplet(*hand, winningTile)) {
        return;
    }

    AddYakuman(yaku, SuuankouName);
}

} // namespace mahjong::internal
