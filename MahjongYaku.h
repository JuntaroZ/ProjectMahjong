#pragma once

#include <string>
#include <vector>

namespace mahjong {

enum class Suit {
    Man,
    Pin,
    Sou,
    Honor
};

enum class Wind {
    East = 1,
    South = 2,
    West = 3,
    North = 4
};

struct Tile {
    Suit suit;
    int rank;

    bool operator==(const Tile& other) const;
};

struct HandContext {
    Tile winningTile{ Suit::Man, 1 };
    Wind seatWind{ Wind::East };
    Wind roundWind{ Wind::East };
    bool isClosed{ true };
    bool isTsumo{ false };
    bool isRiichi{ false };
    bool isIppatsu{ false };
    bool isRinshan{ false };
    bool isChankan{ false };
    bool isHaitei{ false };
    bool isHoutei{ false };
    bool isTenho{ false };
    bool isChiho{ false };
};

struct Yaku {
    std::string name;
    int han;
    bool yakuman{ false };
};

struct YakuResult {
    bool validWinningShape{ false };
    std::vector<Yaku> yaku;
};

Tile Man(int rank);
Tile Pin(int rank);
Tile Sou(int rank);
Tile Honor(int rank);

YakuResult EvaluateYaku(const std::vector<Tile>& closedTiles, const HandContext& context);
YakuResult EvaluateCurrentYaku(const std::vector<Tile>& closedTiles, const HandContext& context);
std::string TileToString(const Tile& tile);

} // namespace mahjong
