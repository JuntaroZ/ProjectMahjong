#include "YakuRules.h"

#include <algorithm>

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
    return std::any_of(hand.melds.begin(), hand.melds.end(), [tile](const Meld& meld) {
        return meld.type == Meld::Type::Triplet && meld.tile == tile;
    });
}

} // namespace

void SanankouRule::Evaluate(const HandAnalysis&, const Decomposition* hand, const HandContext& context, std::vector<Yaku>& yaku) const
{
    if (hand == nullptr) {
        return;
    }

    int concealedTriplets = CountTriplets(*hand);
    const int winningTile = TileIndex(context.winningTile);
    if (!context.isTsumo && HasTriplet(*hand, winningTile)) {
        --concealedTriplets;
    }

    if (concealedTriplets >= 3) {
        Add(yaku, SanankouName, 2);
    }
}

} // namespace mahjong::internal
