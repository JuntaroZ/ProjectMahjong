#include "MahjongYaku.h"

#include "MahjongHandAnalysis.h"
#include "YakuRules/YakuRules.h"

#include <stdexcept>

namespace mahjong {

bool Tile::operator==(const Tile& other) const
{
    return suit == other.suit && rank == other.rank;
}

Tile Man(int rank)
{
    return { Suit::Man, rank };
}

Tile Pin(int rank)
{
    return { Suit::Pin, rank };
}

Tile Sou(int rank)
{
    return { Suit::Sou, rank };
}

Tile Honor(int rank)
{
    return { Suit::Honor, rank };
}

YakuResult EvaluateYaku(const std::vector<Tile>& closedTiles, const HandContext& context)
{
    using namespace internal;

    if (closedTiles.size() != 14) {
        throw std::invalid_argument("EvaluateYaku requires exactly 14 tiles.");
    }

    const HandAnalysis analysis = AnalyzeHand(closedTiles);
    YakuResult result;
    result.validWinningShape = !analysis.standardHands.empty() || analysis.sevenPairs || analysis.kokushi;
    if (!result.validWinningShape) {
        return result;
    }

    const std::vector<std::unique_ptr<YakuRule>> yakumanRules = CreateYakumanRules();
    result.yaku = EvaluateRules(yakumanRules, analysis, nullptr, context);
    if (!result.yaku.empty()) {
        return result;
    }

    const std::vector<std::unique_ptr<YakuRule>> regularRules = CreateRegularRules();
    for (const Decomposition& hand : analysis.standardHands) {
        std::vector<Yaku> candidate = EvaluateRules(regularRules, analysis, &hand, context);
        if (TotalHan(candidate) > TotalHan(result.yaku)) {
            result.yaku = std::move(candidate);
        }
    }

    if (analysis.sevenPairs) {
        std::vector<Yaku> candidate = EvaluateRules(regularRules, analysis, nullptr, context);
        if (TotalHan(candidate) > TotalHan(result.yaku)) {
            result.yaku = std::move(candidate);
        }
    }

    return result;
}

YakuResult EvaluateCurrentYaku(const std::vector<Tile>& closedTiles, const HandContext& context)
{
    using namespace internal;

    if (closedTiles.size() != 14) {
        throw std::invalid_argument("EvaluateCurrentYaku requires exactly 14 tiles.");
    }

    const HandAnalysis analysis = AnalyzeHand(closedTiles);
    YakuResult result;
    result.validWinningShape = !analysis.standardHands.empty() || analysis.sevenPairs || analysis.kokushi;

    const std::vector<std::unique_ptr<YakuRule>> yakumanRules = CreateYakumanRules();
    result.yaku = EvaluateRules(yakumanRules, analysis, nullptr, context);
    if (!result.yaku.empty()) {
        return result;
    }

    const std::vector<std::unique_ptr<YakuRule>> regularRules = CreateRegularRules();
    result.yaku = EvaluateRules(regularRules, analysis, nullptr, context);

    for (const Decomposition& hand : analysis.standardHands) {
        std::vector<Yaku> candidate = EvaluateRules(regularRules, analysis, &hand, context);
        if (TotalHan(candidate) > TotalHan(result.yaku)) {
            result.yaku = std::move(candidate);
        }
    }

    return result;
}

std::string TileToString(const Tile& tile)
{
    const char* suffix = "";
    switch (tile.suit) {
    case Suit::Man:
        suffix = "m";
        break;
    case Suit::Pin:
        suffix = "p";
        break;
    case Suit::Sou:
        suffix = "s";
        break;
    case Suit::Honor:
        suffix = "z";
        break;
    }

    return std::to_string(tile.rank) + suffix;
}

} // namespace mahjong
