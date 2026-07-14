#pragma once

#include <cstddef>
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
    bool allowOpenTanyao{ true };
    bool isIppatsu{ false };
    bool isRinshan{ false };
    bool isChankan{ false };
    bool isHaitei{ false };
    bool isHoutei{ false };
    bool isTenho{ false };
    bool isChiho{ false };
};

enum class MeldType {
    Sequence,
    Triplet,
    Quad,
    OpenQuad
};

struct OpenMeld {
    MeldType type;
    std::vector<Tile> tiles;
};

struct HandState {
    std::vector<Tile> closedTiles;
    std::vector<OpenMeld> openMelds;
    HandContext context;
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

struct YakuScore {
    Yaku yaku;
    int points{ 0 };
    std::string text;
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
YakuResult EvaluateYaku(const HandState& handState);
YakuResult EvaluateCurrentYaku(const std::vector<Tile>& closedTiles, const HandContext& context);
YakuResult EvaluateCurrentYaku(const HandState& handState);
void SetRiichiYaku(bool enabled);
std::vector<Tile> AllTileKinds();
int CountTile(const std::vector<Tile>& tiles, const Tile& target);
bool HasInvalidTileCount(const std::vector<Tile>& tiles);
int CalculateShanten(const std::vector<Tile>& tiles);
int CalculateShanten(const HandState& handState);
bool CanChi(const HandState& handState, std::size_t selectedTile);
bool CanPon(const HandState& handState, std::size_t selectedTile);
bool CanKan(const HandState& handState, std::size_t selectedTile);
bool CanMinkan(const HandState& handState, std::size_t selectedTile);
bool MakeChi(HandState& handState, std::size_t selectedTile);
bool MakePon(HandState& handState, std::size_t selectedTile);
bool MakeKan(HandState& handState, std::size_t selectedTile);
bool MakeMinkan(HandState& handState, std::size_t selectedTile);
bool ReturnOpenMeld(HandState& handState, std::size_t meldIndex);
std::vector<Tile> FindWinningCandidates(const std::vector<Tile>& hand, const HandContext& context);
std::vector<Tile> FindWinningCandidates(const HandState& handState);
std::vector<Yaku> EvaluateDisplayYaku(const std::vector<Tile>& hand, const HandContext& context, const std::vector<Tile>& winningCandidates);
std::vector<Yaku> EvaluateDisplayYaku(const HandState& handState, const std::vector<Tile>& winningCandidates);
std::vector<YakuScore> CalculateDisplayYakuScores(const std::vector<Yaku>& yaku, const HandContext& context);
HandViewAnalysis AnalyzeHandView(const std::vector<Tile>& hand, const HandContext& context);
HandViewAnalysis AnalyzeHandView(const HandState& handState);
std::string TileToString(const Tile& tile);

} // namespace mahjong
