#include "YakuRule.h"

#include <algorithm>

namespace mahjong::internal {

void YakuRule::Add(std::vector<Yaku>& yaku, const std::string& name, int han)
{
    yaku.push_back({ name, han, false });
}

void YakuRule::AddYakuman(std::vector<Yaku>& yaku, const std::string& name)
{
    yaku.push_back({ name, 13, true });
}

bool YakuRule::IsHonor(int tile)
{
    return tile >= 27;
}

bool YakuRule::IsTerminal(int tile)
{
    return tile < 27 && (tile % 9 == 0 || tile % 9 == 8);
}

bool YakuRule::IsTerminalOrHonor(int tile)
{
    return IsTerminal(tile) || IsHonor(tile);
}

bool YakuRule::IsDragon(int tile)
{
    return tile >= 31 && tile <= 33;
}

bool YakuRule::IsWindTile(int tile, Wind wind)
{
    return tile == 27 + static_cast<int>(wind) - 1;
}

bool YakuRule::IsFlushFamily(const HandAnalysis& analysis, bool allowHonors)
{
    int suit = -1;
    for (int tile = 0; tile < TileKindCount; ++tile) {
        if (analysis.counts[tile] == 0) continue;
        if (IsHonor(tile)) {
            if (!allowHonors) return false;
            continue;
        }

        const int currentSuit = tile / 9;
        if (suit == -1) {
            suit = currentSuit;
        } else if (suit != currentSuit) {
            return false;
        }
    }
    return suit != -1;
}

bool YakuRule::AllMeldsAre(const Decomposition& hand, Meld::Type type)
{
    return std::all_of(hand.melds.begin(), hand.melds.end(), [type](const Meld& meld) {
        return meld.type == type;
    });
}

int YakuRule::CountTriplets(const Decomposition& hand)
{
    return static_cast<int>(std::count_if(hand.melds.begin(), hand.melds.end(), [](const Meld& meld) {
        return meld.type == Meld::Type::Triplet;
    }));
}

bool YakuRule::HasSequence(const Decomposition& hand, int tile)
{
    return std::any_of(hand.melds.begin(), hand.melds.end(), [tile](const Meld& meld) {
        return meld.type == Meld::Type::Sequence && meld.tile == tile;
    });
}

} // namespace mahjong::internal
