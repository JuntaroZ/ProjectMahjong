#pragma once

#include "MahjongYaku.h"

#include <array>
#include <vector>

namespace mahjong::internal {

constexpr int TileKindCount = 34;

struct Meld {
    enum class Type {
        Sequence,
        Triplet
    };

    Type type;
    int tile;
};

struct Decomposition {
    int pairTile{ -1 };
    std::vector<Meld> melds;
};

struct HandAnalysis {
    std::array<int, TileKindCount> counts{};
    std::vector<Decomposition> standardHands;
    bool sevenPairs{ false };
    bool kokushi{ false };
};

HandAnalysis AnalyzeHand(const std::vector<Tile>& tiles);
int TotalHan(const std::vector<Yaku>& yaku);

} // namespace mahjong::internal
