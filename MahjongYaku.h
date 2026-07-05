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

struct HandViewAnalysis {
    bool invalidTileCount{ false };
    bool winning{ false };
    bool ready{ false };
    bool riichiOnlyWin{ false };
    int shanten{ 8 };
    std::vector<Yaku> yaku;
    std::vector<Tile> winningCandidates;
};

Tile Man(int rank);
Tile Pin(int rank);
Tile Sou(int rank);
Tile Honor(int rank);

YakuResult EvaluateYaku(const std::vector<Tile>& closedTiles, const HandContext& context);
YakuResult EvaluateCurrentYaku(const std::vector<Tile>& closedTiles, const HandContext& context);
std::vector<Tile> AllTileKinds();
int CountTile(const std::vector<Tile>& tiles, const Tile& target);
bool HasInvalidTileCount(const std::vector<Tile>& tiles);
int CalculateShanten(const std::vector<Tile>& tiles);
std::vector<Tile> FindWinningCandidates(const std::vector<Tile>& hand, const HandContext& context);
std::vector<Yaku> EvaluateDisplayYaku(const std::vector<Tile>& hand, const HandContext& context, const std::vector<Tile>& winningCandidates);
HandViewAnalysis AnalyzeHandView(const std::vector<Tile>& hand, const HandContext& context);
std::string TileToString(const Tile& tile);

} // namespace mahjong
