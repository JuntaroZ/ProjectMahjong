#include "MahjongHandAnalysis.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace mahjong::internal {
namespace {

int ToIndex(const Tile& tile)
{
    if (tile.rank < 1) {
        throw std::invalid_argument("Tile rank must be positive.");
    }

    switch (tile.suit) {
    case Suit::Man:
        if (tile.rank > 9) throw std::invalid_argument("Number tile rank must be 1-9.");
        return tile.rank - 1;
    case Suit::Pin:
        if (tile.rank > 9) throw std::invalid_argument("Number tile rank must be 1-9.");
        return 9 + tile.rank - 1;
    case Suit::Sou:
        if (tile.rank > 9) throw std::invalid_argument("Number tile rank must be 1-9.");
        return 18 + tile.rank - 1;
    case Suit::Honor:
        if (tile.rank > 7) throw std::invalid_argument("Honor tile rank must be 1-7.");
        return 27 + tile.rank - 1;
    }

    throw std::invalid_argument("Unknown tile suit.");
}

bool RemoveMelds(std::array<int, TileKindCount>& counts, std::vector<Meld>& melds)
{
    const auto next = std::find_if(counts.begin(), counts.end(), [](int count) { return count > 0; });
    if (next == counts.end()) {
        return true;
    }

    const int tile = static_cast<int>(std::distance(counts.begin(), next));

    if (counts[tile] >= 3) {
        counts[tile] -= 3;
        melds.push_back({ Meld::Type::Triplet, tile });
        if (RemoveMelds(counts, melds)) return true;
        melds.pop_back();
        counts[tile] += 3;
    }

    const bool canSequence = tile < 27 && tile % 9 <= 6
        && counts[tile + 1] > 0 && counts[tile + 2] > 0;
    if (canSequence) {
        --counts[tile];
        --counts[tile + 1];
        --counts[tile + 2];
        melds.push_back({ Meld::Type::Sequence, tile });
        if (RemoveMelds(counts, melds)) return true;
        melds.pop_back();
        ++counts[tile];
        ++counts[tile + 1];
        ++counts[tile + 2];
    }

    return false;
}

std::array<int, TileKindCount> ToCounts(const std::vector<Tile>& tiles)
{
    std::array<int, TileKindCount> counts{};

    for (const Tile& tile : tiles) {
        const int index = ToIndex(tile);
        ++counts[index];
        if (counts[index] > 4) {
            throw std::invalid_argument("A hand cannot contain more than four of the same tile.");
        }
    }

    return counts;
}

std::vector<Decomposition> FindStandardDecompositions(const std::array<int, TileKindCount>& originalCounts)
{
    std::vector<Decomposition> results;

    for (int pairTile = 0; pairTile < TileKindCount; ++pairTile) {
        if (originalCounts[pairTile] < 2) {
            continue;
        }

        std::array<int, TileKindCount> counts = originalCounts;
        counts[pairTile] -= 2;

        std::vector<Meld> melds;
        if (RemoveMelds(counts, melds)) {
            results.push_back({ pairTile, melds });
        }
    }

    return results;
}

bool IsSevenPairsShape(const std::array<int, TileKindCount>& counts)
{
    int pairs = 0;
    for (int count : counts) {
        if (count == 2) {
            ++pairs;
        } else if (count != 0) {
            return false;
        }
    }
    return pairs == 7;
}

bool IsKokushiShape(const std::array<int, TileKindCount>& counts)
{
    const int requiredTiles[] = { 0, 8, 9, 17, 18, 26, 27, 28, 29, 30, 31, 32, 33 };
    bool hasPair = false;

    for (int tile = 0; tile < TileKindCount; ++tile) {
        const bool required = std::find(std::begin(requiredTiles), std::end(requiredTiles), tile) != std::end(requiredTiles);
        if (!required && counts[tile] > 0) {
            return false;
        }
        if (required) {
            if (counts[tile] == 0) return false;
            if (counts[tile] >= 2) hasPair = true;
        }
    }

    return hasPair;
}

} // namespace

HandAnalysis AnalyzeHand(const std::vector<Tile>& tiles)
{
    HandAnalysis analysis;
    analysis.counts = ToCounts(tiles);
    analysis.standardHands = FindStandardDecompositions(analysis.counts);
    analysis.sevenPairs = IsSevenPairsShape(analysis.counts);
    analysis.kokushi = IsKokushiShape(analysis.counts);
    return analysis;
}

int TotalHan(const std::vector<Yaku>& yaku)
{
    return std::accumulate(yaku.begin(), yaku.end(), 0, [](int sum, const Yaku& item) {
        return sum + item.han;
    });
}

} // namespace mahjong::internal
