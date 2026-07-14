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

bool g_riichiYakuEnabled = false;

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

std::vector<Tile> AllTiles(const HandState& handState)
{
    std::vector<Tile> tiles = handState.closedTiles;
    for (const OpenMeld& meld : handState.openMelds) {
        tiles.insert(tiles.end(), meld.tiles.begin(), meld.tiles.end());
    }
    return tiles;
}

std::size_t EffectiveTileCount(const HandState& handState)
{
    std::size_t count = handState.closedTiles.size();
    for (const OpenMeld& meld : handState.openMelds) {
        count += (meld.type == MeldType::Quad || meld.type == MeldType::OpenQuad) ? 3 : meld.tiles.size();
    }
    return count;
}

HandContext ContextForState(const HandState& handState)
{
    HandContext context = handState.context;
    if (!handState.closedTiles.empty()) {
        context.winningTile = handState.closedTiles.back();
    }
    const bool hasOpenMeld = std::any_of(handState.openMelds.begin(), handState.openMelds.end(), [](const OpenMeld& meld) {
        return meld.type != MeldType::Quad;
    });
    if (hasOpenMeld) {
        context.isClosed = false;
    }
    return context;
}

bool IsSameTileGroup(const std::vector<Tile>& tiles, std::size_t minSize)
{
    if (tiles.size() < minSize || tiles.empty()) {
        return false;
    }
    return std::all_of(tiles.begin(), tiles.end(), [&tiles](const Tile& tile) {
        return tile == tiles.front();
    });
}

int SequenceStartTileIndex(const std::vector<Tile>& tiles)
{
    if (tiles.size() != 3) {
        return -1;
    }

    std::vector<Tile> sorted = tiles;
    std::sort(sorted.begin(), sorted.end(), [](const Tile& left, const Tile& right) {
        return TileKindIndex(left) < TileKindIndex(right);
    });

    if (sorted[0].suit == Suit::Honor || sorted[0].suit != sorted[1].suit || sorted[1].suit != sorted[2].suit) {
        return -1;
    }
    if (sorted[0].rank + 1 != sorted[1].rank || sorted[1].rank + 1 != sorted[2].rank) {
        return -1;
    }

    return TileKindIndex(sorted[0]);
}

std::vector<internal::Meld> FixedMeldsForState(const HandState& handState)
{
    std::vector<internal::Meld> melds;
    melds.reserve(handState.openMelds.size());

    for (const OpenMeld& openMeld : handState.openMelds) {
        switch (openMeld.type) {
        case MeldType::Sequence: {
            const int startIndex = SequenceStartTileIndex(openMeld.tiles);
            if (startIndex < 0) {
                throw std::invalid_argument("Invalid open sequence.");
            }
            melds.push_back({ internal::Meld::Type::Sequence, startIndex, true });
            break;
        }
        case MeldType::Triplet:
            if (!IsSameTileGroup(openMeld.tiles, 3)) {
                throw std::invalid_argument("Invalid open triplet.");
            }
            melds.push_back({ internal::Meld::Type::Triplet, TileKindIndex(openMeld.tiles.front()), true });
            break;
        case MeldType::Quad:
            if (!IsSameTileGroup(openMeld.tiles, 4)) {
                throw std::invalid_argument("Invalid open quad.");
            }
            melds.push_back({ internal::Meld::Type::Triplet, TileKindIndex(openMeld.tiles.front()), false });
            break;
        case MeldType::OpenQuad:
            if (!IsSameTileGroup(openMeld.tiles, 4)) {
                throw std::invalid_argument("Invalid open quad.");
            }
            melds.push_back({ internal::Meld::Type::Triplet, TileKindIndex(openMeld.tiles.front()), true });
            break;
        }
    }

    return melds;
}

void FindClosedMelds(std::array<int, 34>& counts, int neededMelds, std::vector<internal::Meld>& current, std::vector<std::vector<internal::Meld>>& results)
{
    if (neededMelds == 0) {
        if (std::all_of(counts.begin(), counts.end(), [](int count) { return count == 0; })) {
            results.push_back(current);
        }
        return;
    }

    int index = 0;
    while (index < 34 && counts[index] == 0) {
        ++index;
    }
    if (index >= 34) {
        return;
    }

    if (counts[index] >= 3) {
        counts[index] -= 3;
        current.push_back({ internal::Meld::Type::Triplet, index });
        FindClosedMelds(counts, neededMelds - 1, current, results);
        current.pop_back();
        counts[index] += 3;
    }

    if (index < 27 && index % 9 <= 6 && counts[index + 1] > 0 && counts[index + 2] > 0) {
        --counts[index];
        --counts[index + 1];
        --counts[index + 2];
        current.push_back({ internal::Meld::Type::Sequence, index });
        FindClosedMelds(counts, neededMelds - 1, current, results);
        current.pop_back();
        ++counts[index];
        ++counts[index + 1];
        ++counts[index + 2];
    }
}

internal::HandAnalysis AnalyzeHandState(const HandState& handState)
{
    using namespace internal;

    const std::vector<Tile> allTiles = AllTiles(handState);
    if (EffectiveTileCount(handState) != 14 || HasInvalidTileCount(allTiles)) {
        throw std::invalid_argument("Hand analysis requires exactly 14 valid tiles.");
    }
    if (handState.openMelds.empty()) {
        return AnalyzeHand(handState.closedTiles);
    }

    const std::vector<Meld> fixedMelds = FixedMeldsForState(handState);
    if (fixedMelds.size() > 4) {
        throw std::invalid_argument("Too many open melds.");
    }

    HandAnalysis analysis;
    analysis.counts = TileCounts(allTiles);

    const int neededMelds = 4 - static_cast<int>(fixedMelds.size());
    if (handState.closedTiles.size() != static_cast<std::size_t>(neededMelds * 3 + 2)) {
        return analysis;
    }

    std::array<int, 34> closedCounts = TileCounts(handState.closedTiles);
    for (int pair = 0; pair < 34; ++pair) {
        if (closedCounts[pair] < 2) {
            continue;
        }

        closedCounts[pair] -= 2;
        std::vector<Meld> current;
        std::vector<std::vector<Meld>> closedMelds;
        FindClosedMelds(closedCounts, neededMelds, current, closedMelds);
        closedCounts[pair] += 2;

        for (const std::vector<Meld>& melds : closedMelds) {
            Decomposition decomposition;
            decomposition.pairTile = pair;
            decomposition.melds = fixedMelds;
            decomposition.melds.insert(decomposition.melds.end(), melds.begin(), melds.end());
            analysis.standardHands.push_back(std::move(decomposition));
        }
    }

    return analysis;
}

int StandardShantenForState(const HandState& handState)
{
    std::array<int, 34> counts = TileCounts(handState.closedTiles);
    int best = 8;
    SearchStandardShanten(counts, 0, static_cast<int>(handState.openMelds.size()), 0, 0, best);
    return best;
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

void AddRiichiYaku(std::vector<Yaku>& yaku, const HandContext& context)
{
    if (!g_riichiYakuEnabled || !context.isRiichi) {
        return;
    }

    yaku.erase(std::remove_if(yaku.begin(), yaku.end(), [](const Yaku& item) {
        return item.name == u8"立直";
    }), yaku.end());
    yaku.insert(yaku.begin(), { u8"立直", 1, false });
}

void AddUniqueYaku(std::vector<Yaku>& yaku, const Yaku& item)
{
    const auto existing = std::find_if(yaku.begin(), yaku.end(), [&item](const Yaku& current) {
        return current.name == item.name;
    });
    if (existing == yaku.end()) {
        yaku.push_back(item);
    }
}

void AddUniqueYakuResults(std::vector<Yaku>& yaku, const std::vector<Yaku>& items)
{
    for (const Yaku& item : items) {
        AddUniqueYaku(yaku, item);
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
    return EvaluateYaku(HandState{ closedTiles, {}, context });
}

YakuResult EvaluateYaku(const HandState& handState)
{
    using namespace internal;

    if (EffectiveTileCount(handState) != 14) {
        throw std::invalid_argument("EvaluateYaku requires exactly 14 tiles.");
    }

    const HandContext context = ContextForState(handState);
    const HandAnalysis analysis = AnalyzeHandState(handState);
    YakuResult result;
    result.validWinningShape = !analysis.standardHands.empty() || analysis.sevenPairs || analysis.kokushi;
    if (!result.validWinningShape) {
        return result;
    }

    const std::vector<std::unique_ptr<YakuRule>> yakumanRules = CreateYakumanRules();
    AddUniqueYakuResults(result.yaku, EvaluateRules(yakumanRules, analysis, nullptr, context));
    for (const Decomposition& hand : analysis.standardHands) {
        AddUniqueYakuResults(result.yaku, EvaluateRules(yakumanRules, analysis, &hand, context));
    }
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
    return EvaluateCurrentYaku(HandState{ closedTiles, {}, context });
}

YakuResult EvaluateCurrentYaku(const HandState& handState)
{
    using namespace internal;

    if (EffectiveTileCount(handState) != 14) {
        throw std::invalid_argument("EvaluateCurrentYaku requires exactly 14 tiles.");
    }

    const HandContext context = ContextForState(handState);
    const HandAnalysis analysis = AnalyzeHandState(handState);
    YakuResult result;
    result.validWinningShape = !analysis.standardHands.empty() || analysis.sevenPairs || analysis.kokushi;

    const std::vector<std::unique_ptr<YakuRule>> yakumanRules = CreateYakumanRules();
    AddUniqueYakuResults(result.yaku, EvaluateRules(yakumanRules, analysis, nullptr, context));
    for (const Decomposition& hand : analysis.standardHands) {
        AddUniqueYakuResults(result.yaku, EvaluateRules(yakumanRules, analysis, &hand, context));
    }
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

void SetRiichiYaku(bool enabled)
{
    g_riichiYakuEnabled = enabled;
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

bool IsKanMeldType(MeldType type)
{
    return type == MeldType::Quad || type == MeldType::OpenQuad;
}

bool IsSelectedCallableTile(const HandState& handState, std::size_t selectedTile)
{
    return selectedTile < handState.closedTiles.size()
        && !(handState.closedTiles.size() >= 2 && selectedTile == handState.closedTiles.size() - 1);
}

int CountClosedTile(const HandState& handState, const Tile& target)
{
    return CountTile(handState.closedTiles, target);
}

bool RemoveTilesFromClosed(HandState& handState, const std::vector<Tile>& tiles)
{
    std::vector<bool> remove(handState.closedTiles.size(), false);
    for (const Tile& tile : tiles) {
        bool found = false;
        for (std::size_t i = 0; i < handState.closedTiles.size(); ++i) {
            if (!remove[i] && handState.closedTiles[i] == tile) {
                remove[i] = true;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    std::vector<Tile> remaining;
    remaining.reserve(handState.closedTiles.size());
    for (std::size_t i = 0; i < handState.closedTiles.size(); ++i) {
        if (!remove[i]) {
            remaining.push_back(handState.closedTiles[i]);
        }
    }
    handState.closedTiles = std::move(remaining);
    return true;
}

Tile FirstSelectableAddedTile(const HandState& handState)
{
    const std::vector<Tile> currentTiles = AllTiles(handState);
    for (const Tile& tile : AllTileKinds()) {
        if (CountTile(currentTiles, tile) < 4) {
            return tile;
        }
    }
    return Man(1);
}

bool MakeSequenceMeld(HandState& handState, std::size_t selectedTile)
{
    if (!CanChi(handState, selectedTile)) {
        return false;
    }

    const Tile selected = handState.closedTiles[selectedTile];
    for (int start = selected.rank; start >= selected.rank - 2; --start) {
        if (start < 1 || start + 2 > 9) {
            continue;
        }

        std::vector<Tile> tiles = {
            { selected.suit, start },
            { selected.suit, start + 1 },
            { selected.suit, start + 2 }
        };

        bool possible = true;
        for (const Tile& tile : tiles) {
            if (CountClosedTile(handState, tile) < 1) {
                possible = false;
                break;
            }
        }

        if (possible) {
            if (!RemoveTilesFromClosed(handState, tiles)) {
                return false;
            }
            handState.openMelds.push_back({ MeldType::Sequence, std::move(tiles) });
            return true;
        }
    }

    return false;
}

bool MakeSameTileMeld(HandState& handState, std::size_t selectedTile, MeldType type, int count)
{
    if (!IsSelectedCallableTile(handState, selectedTile)) {
        return false;
    }

    const Tile selected = handState.closedTiles[selectedTile];
    if (CountClosedTile(handState, selected) < count) {
        return false;
    }

    std::vector<Tile> tiles(static_cast<std::size_t>(count), selected);
    if (!RemoveTilesFromClosed(handState, tiles)) {
        return false;
    }

    handState.openMelds.push_back({ type, tiles });
    if (IsKanMeldType(type)) {
        handState.closedTiles.push_back(FirstSelectableAddedTile(handState));
    }
    return true;
}

int CalculateShanten(const std::vector<Tile>& tiles)
{
    const std::array<int, 34> counts = TileCounts(tiles);
    return std::min({ StandardShanten(counts), SevenPairsShanten(counts), KokushiShanten(counts) });
}

int CalculateShanten(const HandState& handState)
{
    const std::vector<Tile> allTiles = AllTiles(handState);
    if (handState.openMelds.empty()) {
        return CalculateShanten(allTiles);
    }
    return StandardShantenForState(handState);
}

bool CanChi(const HandState& handState, std::size_t selectedTile)
{
    if (!IsSelectedCallableTile(handState, selectedTile)) {
        return false;
    }

    const Tile selected = handState.closedTiles[selectedTile];
    if (selected.suit == Suit::Honor) {
        return false;
    }

    for (int start = selected.rank; start >= selected.rank - 2; --start) {
        if (start < 1 || start + 2 > 9) {
            continue;
        }

        const std::vector<Tile> needed = {
            { selected.suit, start },
            { selected.suit, start + 1 },
            { selected.suit, start + 2 }
        };

        bool possible = true;
        for (const Tile& tile : needed) {
            if (CountClosedTile(handState, tile) < 1) {
                possible = false;
                break;
            }
        }
        if (possible) {
            return true;
        }
    }

    return false;
}

bool CanPon(const HandState& handState, std::size_t selectedTile)
{
    return IsSelectedCallableTile(handState, selectedTile)
        && CountClosedTile(handState, handState.closedTiles[selectedTile]) >= 3;
}

bool CanKan(const HandState& handState, std::size_t selectedTile)
{
    return IsSelectedCallableTile(handState, selectedTile)
        && CountClosedTile(handState, handState.closedTiles[selectedTile]) >= 4;
}

bool CanMinkan(const HandState& handState, std::size_t selectedTile)
{
    return CanKan(handState, selectedTile);
}

bool MakeChi(HandState& handState, std::size_t selectedTile)
{
    return MakeSequenceMeld(handState, selectedTile);
}

bool MakePon(HandState& handState, std::size_t selectedTile)
{
    return MakeSameTileMeld(handState, selectedTile, MeldType::Triplet, 3);
}

bool MakeKan(HandState& handState, std::size_t selectedTile)
{
    return MakeSameTileMeld(handState, selectedTile, MeldType::Quad, 4);
}

bool MakeMinkan(HandState& handState, std::size_t selectedTile)
{
    return MakeSameTileMeld(handState, selectedTile, MeldType::OpenQuad, 4);
}

bool ReturnOpenMeld(HandState& handState, std::size_t meldIndex)
{
    if (meldIndex >= handState.openMelds.size()) {
        return false;
    }

    const OpenMeld returningMeld = handState.openMelds[meldIndex];
    if (IsKanMeldType(returningMeld.type) && !handState.closedTiles.empty()) {
        handState.closedTiles.pop_back();
    }

    const auto insertPosition = handState.closedTiles.empty() ? handState.closedTiles.end() : handState.closedTiles.end() - 1;
    handState.closedTiles.insert(insertPosition, returningMeld.tiles.begin(), returningMeld.tiles.end());
    handState.openMelds.erase(handState.openMelds.begin() + static_cast<std::ptrdiff_t>(meldIndex));
    return true;
}

std::vector<Tile> FindWinningCandidates(const std::vector<Tile>& hand, const HandContext& context)
{
    return FindWinningCandidates(HandState{ hand, {}, context });
}

std::vector<Tile> FindWinningCandidates(const HandState& handState)
{
    std::vector<Tile> winningCandidates;
    const std::vector<Tile> allTiles = AllTiles(handState);
    if (EffectiveTileCount(handState) != 14 || HasInvalidTileCount(allTiles) || handState.closedTiles.empty()) {
        return winningCandidates;
    }

    const std::vector<Tile> baseClosedTiles(handState.closedTiles.begin(), handState.closedTiles.end() - 1);
    for (const Tile& candidate : AllTileKinds()) {
        std::vector<Tile> baseAllTiles = baseClosedTiles;
        for (const OpenMeld& meld : handState.openMelds) {
            baseAllTiles.insert(baseAllTiles.end(), meld.tiles.begin(), meld.tiles.end());
        }
        if (CountTile(baseAllTiles, candidate) >= 4) {
            continue;
        }

        HandState testState = handState;
        testState.closedTiles = baseClosedTiles;
        testState.closedTiles.push_back(candidate);

        testState.context.winningTile = candidate;

        try {
            const YakuResult result = EvaluateYaku(testState);
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
    return EvaluateDisplayYaku(HandState{ hand, {}, context }, winningCandidates);
}

std::vector<Yaku> EvaluateDisplayYaku(const HandState& handState, const std::vector<Tile>& winningCandidates)
{
    std::vector<Yaku> yaku;
    const std::vector<Tile> allTiles = AllTiles(handState);

    try {
        const YakuResult currentResult = EvaluateYaku(handState);
        if (currentResult.validWinningShape) {
            AddYakuResultNames(yaku, currentResult);
            AddRiichiYaku(yaku, ContextForState(handState));
            return yaku;
        }
    }
    catch (...) {
    }

    if (handState.context.isRiichi && EffectiveTileCount(handState) == 14 && !handState.closedTiles.empty()) {
        const std::vector<Tile> baseClosedTiles(handState.closedTiles.begin(), handState.closedTiles.end() - 1);
        for (const Tile& candidate : winningCandidates) {
            HandState testState = handState;
            testState.closedTiles = baseClosedTiles;
            testState.closedTiles.push_back(candidate);
            testState.context.winningTile = candidate;

            try {
                AddYakuResultNames(yaku, EvaluateYaku(testState));
            }
            catch (...) {
            }
        }
    }

    try {
        AddYakuResultNames(yaku, EvaluateCurrentYaku(handState));
    }
    catch (...) {
    }

    AddRiichiYaku(yaku, ContextForState(handState));
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
    return AnalyzeHandView(HandState{ hand, {}, context });
}

HandViewAnalysis AnalyzeHandView(const HandState& handState)
{
    HandViewAnalysis analysis;
    const std::vector<Tile> allTiles = AllTiles(handState);
    analysis.invalidTileCount = HasInvalidTileCount(allTiles);
    if (analysis.invalidTileCount) {
        return analysis;
    }

    const HandContext currentContext = ContextForState(handState);

    try {
        analysis.shanten = CalculateShanten(handState);
    }
    catch (...) {
        analysis.shanten = 8;
    }

    try {
        const YakuResult currentResult = EvaluateYaku(handState);
        analysis.winning = currentResult.validWinningShape;
    }
    catch (...) {
    }

    analysis.winningCandidates = FindWinningCandidates(handState);
    analysis.ready = !analysis.winningCandidates.empty();
    HandState currentState = handState;
    currentState.context = currentContext;
    analysis.yaku = EvaluateDisplayYaku(currentState, analysis.winningCandidates);

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

YakuResult MahjongLibrary::EvaluateYaku(const std::vector<Tile>& closedTiles, const HandContext& context) const
{
    return mahjong::EvaluateYaku(closedTiles, context);
}

YakuResult MahjongLibrary::EvaluateYaku(const HandState& handState) const
{
    return mahjong::EvaluateYaku(handState);
}

YakuResult MahjongLibrary::EvaluateCurrentYaku(const std::vector<Tile>& closedTiles, const HandContext& context) const
{
    return mahjong::EvaluateCurrentYaku(closedTiles, context);
}

YakuResult MahjongLibrary::EvaluateCurrentYaku(const HandState& handState) const
{
    return mahjong::EvaluateCurrentYaku(handState);
}

void MahjongLibrary::SetRiichiYaku(bool enabled) const
{
    mahjong::SetRiichiYaku(enabled);
}

int MahjongLibrary::CalculateShanten(const std::vector<Tile>& tiles) const
{
    return mahjong::CalculateShanten(tiles);
}

int MahjongLibrary::CalculateShanten(const HandState& handState) const
{
    return mahjong::CalculateShanten(handState);
}

bool MahjongLibrary::CanChi(const HandState& handState, std::size_t selectedTile) const
{
    return mahjong::CanChi(handState, selectedTile);
}

bool MahjongLibrary::CanPon(const HandState& handState, std::size_t selectedTile) const
{
    return mahjong::CanPon(handState, selectedTile);
}

bool MahjongLibrary::CanKan(const HandState& handState, std::size_t selectedTile) const
{
    return mahjong::CanKan(handState, selectedTile);
}

bool MahjongLibrary::CanMinkan(const HandState& handState, std::size_t selectedTile) const
{
    return mahjong::CanMinkan(handState, selectedTile);
}

bool MahjongLibrary::MakeChi(HandState& handState, std::size_t selectedTile) const
{
    return mahjong::MakeChi(handState, selectedTile);
}

bool MahjongLibrary::MakePon(HandState& handState, std::size_t selectedTile) const
{
    return mahjong::MakePon(handState, selectedTile);
}

bool MahjongLibrary::MakeKan(HandState& handState, std::size_t selectedTile) const
{
    return mahjong::MakeKan(handState, selectedTile);
}

bool MahjongLibrary::MakeMinkan(HandState& handState, std::size_t selectedTile) const
{
    return mahjong::MakeMinkan(handState, selectedTile);
}

bool MahjongLibrary::ReturnOpenMeld(HandState& handState, std::size_t meldIndex) const
{
    return mahjong::ReturnOpenMeld(handState, meldIndex);
}

std::vector<Tile> MahjongLibrary::FindWinningCandidates(const std::vector<Tile>& hand, const HandContext& context) const
{
    return mahjong::FindWinningCandidates(hand, context);
}

std::vector<Tile> MahjongLibrary::FindWinningCandidates(const HandState& handState) const
{
    return mahjong::FindWinningCandidates(handState);
}

std::vector<Yaku> MahjongLibrary::EvaluateDisplayYaku(
    const std::vector<Tile>& hand,
    const HandContext& context,
    const std::vector<Tile>& winningCandidates) const
{
    return mahjong::EvaluateDisplayYaku(hand, context, winningCandidates);
}

std::vector<Yaku> MahjongLibrary::EvaluateDisplayYaku(
    const HandState& handState,
    const std::vector<Tile>& winningCandidates) const
{
    return mahjong::EvaluateDisplayYaku(handState, winningCandidates);
}

std::vector<YakuScore> MahjongLibrary::CalculateDisplayYakuScores(const std::vector<Yaku>& yaku, const HandContext& context) const
{
    return mahjong::CalculateDisplayYakuScores(yaku, context);
}

HandViewAnalysis MahjongLibrary::AnalyzeHandView(const std::vector<Tile>& hand, const HandContext& context) const
{
    return mahjong::AnalyzeHandView(hand, context);
}

HandViewAnalysis MahjongLibrary::AnalyzeHandView(const HandState& handState) const
{
    return mahjong::AnalyzeHandView(handState);
}

} // namespace mahjong
