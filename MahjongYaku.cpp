#include "MahjongYaku.h"

#include "MahjongHandAnalysis.h"
#include "YakuRules/YakuRules.h"

#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <sstream>

namespace mahjong {
namespace {

int TileKindIndex(const Tile& tile)
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
    return 0;
}

std::array<int, 34> TileCounts(const std::vector<Tile>& tiles)
{
    std::array<int, 34> counts{};
    for (const Tile& tile : tiles) {
        ++counts[TileKindIndex(tile)];
    }
    return counts;
}

void SearchStandardShanten(std::array<int, 34>& counts, int index, int melds, int pairs, int taatsu, int& best)
{
    while (index < 34 && counts[index] == 0) {
        ++index;
    }
    if (index >= 34) {
        taatsu = std::min(taatsu, 4 - melds);
        best = std::min(best, 8 - melds * 2 - taatsu - pairs);
        return;
    }

    if (counts[index] >= 3) {
        counts[index] -= 3;
        SearchStandardShanten(counts, index, melds + 1, pairs, taatsu, best);
        counts[index] += 3;
    }

    if (index < 27 && index % 9 <= 6 && counts[index + 1] > 0 && counts[index + 2] > 0) {
        --counts[index];
        --counts[index + 1];
        --counts[index + 2];
        SearchStandardShanten(counts, index, melds + 1, pairs, taatsu, best);
        ++counts[index];
        ++counts[index + 1];
        ++counts[index + 2];
    }

    if (counts[index] >= 2) {
        counts[index] -= 2;
        SearchStandardShanten(counts, index, melds, pairs + 1, taatsu, best);
        counts[index] += 2;
    }

    if (index < 27 && index % 9 <= 7 && counts[index + 1] > 0) {
        --counts[index];
        --counts[index + 1];
        SearchStandardShanten(counts, index, melds, pairs, taatsu + 1, best);
        ++counts[index];
        ++counts[index + 1];
    }

    if (index < 27 && index % 9 <= 6 && counts[index + 2] > 0) {
        --counts[index];
        --counts[index + 2];
        SearchStandardShanten(counts, index, melds, pairs, taatsu + 1, best);
        ++counts[index];
        ++counts[index + 2];
    }

    --counts[index];
    SearchStandardShanten(counts, index, melds, pairs, taatsu, best);
    ++counts[index];
}

int StandardShanten(const std::array<int, 34>& originalCounts)
{
    std::array<int, 34> counts = originalCounts;
    int best = 8;
    SearchStandardShanten(counts, 0, 0, 0, 0, best);
    return best;
}

int SevenPairsShanten(const std::array<int, 34>& counts)
{
    int pairs = 0;
    int kinds = 0;
    for (int count : counts) {
        if (count > 0) {
            ++kinds;
        }
        if (count >= 2) {
            ++pairs;
        }
    }

    return 6 - pairs + std::max(0, 7 - kinds);
}

int KokushiShanten(const std::array<int, 34>& counts)
{
    const int requiredTiles[] = { 0, 8, 9, 17, 18, 26, 27, 28, 29, 30, 31, 32, 33 };
    int kinds = 0;
    bool hasPair = false;

    for (int tile : requiredTiles) {
        if (counts[tile] > 0) {
            ++kinds;
        }
        if (counts[tile] >= 2) {
            hasPair = true;
        }
    }

    return 13 - kinds - (hasPair ? 1 : 0);
}

void AddDisplayYakuName(std::vector<Yaku>& yaku, const Yaku& item)
{
    if (item.name == u8"立直") {
        return;
    }
    const auto existing = std::find_if(yaku.begin(), yaku.end(), [&item](const Yaku& current) {
        return current.name == item.name;
    });
    if (existing == yaku.end()) {
        yaku.push_back(item);
    }
}

void AddYakuResultNames(std::vector<Yaku>& yaku, const YakuResult& result)
{
    for (const Yaku& item : result.yaku) {
        AddDisplayYakuName(yaku, item);
    }
}

int RoundUp100(int value)
{
    return ((value + 99) / 100) * 100;
}

int DisplayFuForYaku(const Yaku& yaku, const HandContext& context)
{
    if (yaku.name == u8"七対子") {
        return 25;
    }
    if (yaku.name == u8"平和" && context.isTsumo) {
        return 20;
    }
    return 30;
}

int BasePointsForYaku(const Yaku& yaku, const HandContext& context)
{
    if (yaku.yakuman) {
        return 8000;
    }

    const int han = yaku.han;
    if (han >= 13) {
        return 8000;
    }
    if (han >= 11) {
        return 6000;
    }
    if (han >= 8) {
        return 4000;
    }
    if (han >= 6) {
        return 3000;
    }

    const int fu = DisplayFuForYaku(yaku, context);
    int basePoints = fu;
    for (int i = 0; i < han + 2; ++i) {
        basePoints *= 2;
    }

    if (han >= 5 || basePoints >= 2000) {
        return 2000;
    }
    return basePoints;
}

int DisplayScorePointsForYaku(const Yaku& yaku, const HandContext& context)
{
    return RoundUp100(BasePointsForYaku(yaku, context) * 4);
}

std::string ScoreTextForYaku(const Yaku& yaku, const HandContext& context)
{
    const int points = DisplayScorePointsForYaku(yaku, context);
    std::ostringstream text;

    text << yaku.han << (yaku.yakuman ? u8"翻（役満）" : u8"翻") << u8" / ";
    text << points << u8"点";
    return text.str();
}

} // namespace

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

std::vector<Tile> AllTileKinds()
{
    std::vector<Tile> tiles;
    for (int rank = 1; rank <= 9; ++rank) {
        tiles.push_back(Man(rank));
    }
    for (int rank = 1; rank <= 9; ++rank) {
        tiles.push_back(Pin(rank));
    }
    for (int rank = 1; rank <= 9; ++rank) {
        tiles.push_back(Sou(rank));
    }
    for (int rank = 1; rank <= 7; ++rank) {
        tiles.push_back(Honor(rank));
    }
    return tiles;
}

int CountTile(const std::vector<Tile>& tiles, const Tile& target)
{
    return static_cast<int>(std::count(tiles.begin(), tiles.end(), target));
}

bool HasInvalidTileCount(const std::vector<Tile>& tiles)
{
    for (const Tile& tile : AllTileKinds()) {
        if (CountTile(tiles, tile) > 4) {
            return true;
        }
    }
    return false;
}

int CalculateShanten(const std::vector<Tile>& tiles)
{
    const std::array<int, 34> counts = TileCounts(tiles);
    return std::min({ StandardShanten(counts), SevenPairsShanten(counts), KokushiShanten(counts) });
}

std::vector<Tile> FindWinningCandidates(const std::vector<Tile>& hand, const HandContext& context)
{
    std::vector<Tile> winningCandidates;
    if (hand.size() != 14 || HasInvalidTileCount(hand)) {
        return winningCandidates;
    }

    const std::vector<Tile> baseHand(hand.begin(), hand.end() - 1);
    for (const Tile& candidate : AllTileKinds()) {
        if (CountTile(baseHand, candidate) >= 4) {
            continue;
        }

        std::vector<Tile> testHand = baseHand;
        testHand.push_back(candidate);

        HandContext candidateContext = context;
        candidateContext.winningTile = candidate;

        try {
            const YakuResult result = EvaluateYaku(testHand, candidateContext);
            if (result.validWinningShape) {
                winningCandidates.push_back(candidate);
            }
        }
        catch (...) {
        }
    }

    return winningCandidates;
}

std::vector<Yaku> EvaluateDisplayYaku(const std::vector<Tile>& hand, const HandContext& context, const std::vector<Tile>& winningCandidates)
{
    std::vector<Yaku> yaku;

    try {
        HandContext currentContext = context;
        if (!hand.empty()) {
            currentContext.winningTile = hand.back();
        }

        const YakuResult currentResult = EvaluateYaku(hand, currentContext);
        if (currentResult.validWinningShape) {
            AddYakuResultNames(yaku, currentResult);
            return yaku;
        }
    }
    catch (...) {
    }

    if (context.isRiichi && hand.size() == 14) {
        const std::vector<Tile> baseHand(hand.begin(), hand.end() - 1);
        for (const Tile& candidate : winningCandidates) {
            std::vector<Tile> testHand = baseHand;
            testHand.push_back(candidate);

            HandContext candidateContext = context;
            candidateContext.winningTile = candidate;

            try {
                AddYakuResultNames(yaku, EvaluateYaku(testHand, candidateContext));
            }
            catch (...) {
            }
        }
    }

    try {
        HandContext currentContext = context;
        if (!hand.empty()) {
            currentContext.winningTile = hand.back();
        }
        AddYakuResultNames(yaku, EvaluateCurrentYaku(hand, currentContext));
    }
    catch (...) {
    }

    return yaku;
}

std::vector<YakuScore> CalculateDisplayYakuScores(const std::vector<Yaku>& yaku, const HandContext& context)
{
    std::vector<YakuScore> scores;
    scores.reserve(yaku.size());

    for (const Yaku& item : yaku) {
        YakuScore score;
        score.yaku = item;
        score.points = DisplayScorePointsForYaku(item, context);
        score.text = ScoreTextForYaku(item, context);
        scores.push_back(std::move(score));
    }

    return scores;
}

HandViewAnalysis AnalyzeHandView(const std::vector<Tile>& hand, const HandContext& context)
{
    HandViewAnalysis analysis;
    analysis.invalidTileCount = HasInvalidTileCount(hand);
    if (analysis.invalidTileCount) {
        return analysis;
    }

    HandContext currentContext = context;
    if (!hand.empty()) {
        currentContext.winningTile = hand.back();
    }

    try {
        analysis.shanten = CalculateShanten(hand);
    }
    catch (...) {
        analysis.shanten = 8;
    }

    try {
        const YakuResult currentResult = EvaluateYaku(hand, currentContext);
        analysis.winning = currentResult.validWinningShape;
    }
    catch (...) {
    }

    if (context.isRiichi) {
        analysis.winningCandidates = FindWinningCandidates(hand, context);
    }
    analysis.ready = !analysis.winningCandidates.empty();
    analysis.yaku = EvaluateDisplayYaku(hand, currentContext, analysis.winningCandidates);

    if (analysis.winning) {
        analysis.riichiOnlyWin = analysis.yaku.empty();
    }

    return analysis;
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
