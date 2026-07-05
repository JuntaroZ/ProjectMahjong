#include "MahjongYaku.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace {

constexpr int TileGap = 6;
constexpr int TsumoGap = 22;
constexpr int CandidateAreaGap = 28;
constexpr int CandidateGap = 4;
constexpr int CandidateFramePadding = 6;
constexpr int CandidateRowGap = 42;
constexpr int MaxWinningCandidateCount = 13;
constexpr int WindowPadding = 16;
constexpr int YakuAreaHeight = 66;
constexpr int SelectionPadding = 6;
constexpr int SelectionPenWidth = 3;

struct TileImage {
    std::wstring fileName;
    std::unique_ptr<Gdiplus::Image> image;
};

std::vector<TileImage> g_tileImages;
std::vector<TileImage> g_candidateImages;
std::vector<mahjong::Tile> g_hand;
std::vector<mahjong::Tile> g_winningCandidates;
mahjong::HandContext g_context;
std::wstring g_yakuText;
std::wstring g_shantenText;
bool g_invalidTileSelection = false;
bool g_isRonTile = false;
bool g_tsumoRonLabelTouched = false;
std::filesystem::path g_imageDirectory;
std::size_t g_selectedTile = 0;
ULONG_PTR g_gdiplusToken = 0;

int CalculateShanten(const std::vector<mahjong::Tile>& tiles);
std::vector<std::wstring> CollectDisplayYakuNames();
RECT TileImageRect(std::size_t index);

int TileTop()
{
    return WindowPadding + YakuAreaHeight;
}

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty()) {
        return {};
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 1) {
        return {};
    }

    std::wstring wide(length - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), length);
    return wide;
}

void UpdateYakuText()
{
    if (g_invalidTileSelection) {
        g_yakuText = L"4個以上は選べません";
        return;
    }

    try {
        const std::vector<std::wstring> yakuNames = CollectDisplayYakuNames();

        if (yakuNames.empty()) {
            g_yakuText = L"成立役: なし";
            return;
        }

        g_yakuText = L"成立役: ";
        for (std::size_t i = 0; i < yakuNames.size(); ++i) {
            if (i > 0) {
                g_yakuText += L" / ";
            }
            g_yakuText += yakuNames[i];
        }
    }
    catch (...) {
        g_yakuText = L"成立役: なし";
    }
}

void UpdateShantenText()
{
    if (g_invalidTileSelection) {
        g_shantenText.clear();
        return;
    }

    try {
        const int shanten = CalculateShanten(g_hand);
        if (shanten < 0) {
            const mahjong::YakuResult result = mahjong::EvaluateCurrentYaku(g_hand, g_context);
            const bool hasNonRiichiYaku = std::any_of(result.yaku.begin(), result.yaku.end(), [](const mahjong::Yaku& yaku) {
                return yaku.name != u8"立直";
            });
            g_shantenText = hasNonRiichiYaku ? L"和了" : L"和了（リーチのみ）";
        }
        else if (g_context.isRiichi && !g_winningCandidates.empty()) {
            g_shantenText = L"テンパイ";
        }
        else if (shanten == 1) {
            g_shantenText = L"イーシャンテン";
        }
        else if (shanten == 2) {
            g_shantenText = L"リャンシャンテン";
        }
        else {
            g_shantenText.clear();
        }
    }
    catch (...) {
        g_shantenText.clear();
    }
}

std::vector<mahjong::Tile> AllTileKinds()
{
    std::vector<mahjong::Tile> tiles;
    for (int rank = 1; rank <= 9; ++rank) {
        tiles.push_back(mahjong::Man(rank));
    }
    for (int rank = 1; rank <= 9; ++rank) {
        tiles.push_back(mahjong::Pin(rank));
    }
    for (int rank = 1; rank <= 9; ++rank) {
        tiles.push_back(mahjong::Sou(rank));
    }
    for (int rank = 1; rank <= 7; ++rank) {
        tiles.push_back(mahjong::Honor(rank));
    }
    return tiles;
}

int CountTile(const std::vector<mahjong::Tile>& tiles, const mahjong::Tile& target)
{
    return static_cast<int>(std::count(tiles.begin(), tiles.end(), target));
}

int TileKindIndex(const mahjong::Tile& tile)
{
    switch (tile.suit) {
    case mahjong::Suit::Man:
        return tile.rank - 1;
    case mahjong::Suit::Pin:
        return 9 + tile.rank - 1;
    case mahjong::Suit::Sou:
        return 18 + tile.rank - 1;
    case mahjong::Suit::Honor:
        return 27 + tile.rank - 1;
    }
    return 0;
}

std::array<int, 34> TileCounts(const std::vector<mahjong::Tile>& tiles)
{
    std::array<int, 34> counts{};
    for (const mahjong::Tile& tile : tiles) {
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

int CalculateShanten(const std::vector<mahjong::Tile>& tiles)
{
    const std::array<int, 34> counts = TileCounts(tiles);
    return std::min({ StandardShanten(counts), SevenPairsShanten(counts), KokushiShanten(counts) });
}

void UpdateInvalidTileSelection()
{
    g_invalidTileSelection = false;
    if (g_selectedTile < g_hand.size()) {
        g_invalidTileSelection = CountTile(g_hand, g_hand[g_selectedTile]) > 4;
    }
}

void UpdateWinningTileFromHand()
{
    if (!g_hand.empty()) {
        g_context.winningTile = g_hand.back();
    }
    g_context.isTsumo = !g_isRonTile;
}

std::wstring TileImageFileName(const mahjong::Tile& tile)
{
    wchar_t suit = L'z';
    switch (tile.suit) {
    case mahjong::Suit::Man:
        suit = L'm';
        break;
    case mahjong::Suit::Pin:
        suit = L'p';
        break;
    case mahjong::Suit::Sou:
        suit = L's';
        break;
    case mahjong::Suit::Honor:
        suit = L'z';
        break;
    }

    std::wstringstream fileName;
    fileName << suit << tile.rank << L".png";
    return fileName.str();
}

std::filesystem::path ExecutableDirectory()
{
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = 0;

    while (true) {
        length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return std::filesystem::current_path();
        }
        if (length < buffer.size() - 1) {
            buffer.resize(length);
            break;
        }
        buffer.resize(buffer.size() * 2);
    }

    return std::filesystem::path(buffer).parent_path();
}

std::filesystem::path FindTileImageDirectory()
{
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::current_path() / L"Image" / L"Pai",
        ExecutableDirectory() / L"Image" / L"Pai",
        ExecutableDirectory() / L".." / L".." / L"Image" / L"Pai"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::weakly_canonical(candidate);
        }
    }

    return candidates.front();
}

void LoadTileImages(const std::vector<mahjong::Tile>& hand)
{
    g_imageDirectory = FindTileImageDirectory();
    g_tileImages.clear();
    g_tileImages.reserve(hand.size());

    for (const mahjong::Tile& tile : hand) {
        const std::wstring fileName = TileImageFileName(tile);
        auto image = std::make_unique<Gdiplus::Image>((g_imageDirectory / fileName).c_str());
        g_tileImages.push_back({ fileName, std::move(image) });
    }
}

void LoadTileImage(std::size_t index)
{
    if (index >= g_hand.size() || index >= g_tileImages.size()) {
        return;
    }

    const std::wstring fileName = TileImageFileName(g_hand[index]);
    auto image = std::make_unique<Gdiplus::Image>((g_imageDirectory / fileName).c_str());
    g_tileImages[index] = { fileName, std::move(image) };
}

void LoadWinningCandidateImages()
{
    g_candidateImages.clear();
    g_candidateImages.reserve(g_winningCandidates.size());

    for (const mahjong::Tile& tile : g_winningCandidates) {
        const std::wstring fileName = TileImageFileName(tile);
        auto image = std::make_unique<Gdiplus::Image>((g_imageDirectory / fileName).c_str());
        g_candidateImages.push_back({ fileName, std::move(image) });
    }
}

void UpdateWinningCandidates()
{
    g_winningCandidates.clear();
    if (g_invalidTileSelection || !g_context.isRiichi || g_hand.size() != 14) {
        LoadWinningCandidateImages();
        return;
    }

    std::vector<mahjong::Tile> baseHand(g_hand.begin(), g_hand.end() - 1);
    for (const mahjong::Tile& candidate : AllTileKinds()) {
        if (CountTile(baseHand, candidate) >= 4) {
            continue;
        }

        std::vector<mahjong::Tile> testHand = baseHand;
        testHand.push_back(candidate);

        mahjong::HandContext candidateContext = g_context;
        candidateContext.winningTile = candidate;

        try {
            const mahjong::YakuResult result = mahjong::EvaluateYaku(testHand, candidateContext);
            if (result.validWinningShape) {
                g_winningCandidates.push_back(candidate);
            }
        }
        catch (...) {
        }
    }

    LoadWinningCandidateImages();
}

void AddDisplayYakuName(std::vector<std::string>& names, const mahjong::Yaku& yaku)
{
    if (yaku.name == u8"立直") {
        return;
    }
    if (std::find(names.begin(), names.end(), yaku.name) != names.end()) {
        return;
    }

    names.push_back(yaku.name);
}

void AddYakuResultNames(std::vector<std::string>& names, const mahjong::YakuResult& result)
{
    for (const mahjong::Yaku& yaku : result.yaku) {
        AddDisplayYakuName(names, yaku);
    }
}

std::vector<std::wstring> CollectDisplayYakuNames()
{
    std::vector<std::string> names;

    mahjong::YakuResult currentResult;
    bool hasCurrentResult = false;

    try {
        mahjong::HandContext currentContext = g_context;
        if (!g_hand.empty()) {
            currentContext.winningTile = g_hand.back();
        }

        currentResult = mahjong::EvaluateYaku(g_hand, currentContext);
        hasCurrentResult = true;
        if (currentResult.validWinningShape) {
            AddYakuResultNames(names, currentResult);
        }
    }
    catch (...) {
    }

    if (hasCurrentResult && currentResult.validWinningShape) {
        std::vector<std::wstring> wideNames;
        wideNames.reserve(names.size());
        for (const std::string& name : names) {
            wideNames.push_back(Utf8ToWide(name));
        }
        return wideNames;
    }

    if (g_context.isRiichi && g_hand.size() == 14) {
        const std::vector<mahjong::Tile> baseHand(g_hand.begin(), g_hand.end() - 1);
        for (const mahjong::Tile& candidate : g_winningCandidates) {
            std::vector<mahjong::Tile> testHand = baseHand;
            testHand.push_back(candidate);

            mahjong::HandContext candidateContext = g_context;
            candidateContext.winningTile = candidate;

            try {
                AddYakuResultNames(names, mahjong::EvaluateYaku(testHand, candidateContext));
            }
            catch (...) {
            }
        }
    }

    try {
        mahjong::HandContext currentContext = g_context;
        if (!g_hand.empty()) {
            currentContext.winningTile = g_hand.back();
        }

        AddYakuResultNames(names, mahjong::EvaluateCurrentYaku(g_hand, currentContext));
    }
    catch (...) {
    }

    std::vector<std::wstring> wideNames;
    wideNames.reserve(names.size());
    for (const std::string& name : names) {
        wideNames.push_back(Utf8ToWide(name));
    }
    return wideNames;
}

int TileRankLimit(mahjong::Suit suit)
{
    return suit == mahjong::Suit::Honor ? 7 : 9;
}

mahjong::Suit NextSuit(mahjong::Suit suit)
{
    switch (suit) {
    case mahjong::Suit::Man:
        return mahjong::Suit::Pin;
    case mahjong::Suit::Pin:
        return mahjong::Suit::Sou;
    case mahjong::Suit::Sou:
        return mahjong::Suit::Honor;
    case mahjong::Suit::Honor:
        return mahjong::Suit::Man;
    }

    return mahjong::Suit::Man;
}

mahjong::Suit PreviousSuit(mahjong::Suit suit)
{
    switch (suit) {
    case mahjong::Suit::Man:
        return mahjong::Suit::Honor;
    case mahjong::Suit::Pin:
        return mahjong::Suit::Man;
    case mahjong::Suit::Sou:
        return mahjong::Suit::Pin;
    case mahjong::Suit::Honor:
        return mahjong::Suit::Sou;
    }

    return mahjong::Suit::Man;
}

void MoveSelectedTile(int direction)
{
    if (g_hand.empty()) {
        return;
    }

    const int tileCount = static_cast<int>(g_hand.size());
    const int nextIndex = (static_cast<int>(g_selectedTile) + tileCount + direction) % tileCount;
    g_selectedTile = static_cast<std::size_t>(nextIndex);
}

void ChangeSelectedTileRank(int direction)
{
    if (g_selectedTile >= g_hand.size()) {
        return;
    }

    mahjong::Tile& tile = g_hand[g_selectedTile];
    tile.rank += direction;

    if (tile.rank > TileRankLimit(tile.suit)) {
        tile.suit = NextSuit(tile.suit);
        tile.rank = 1;
    }
    else if (tile.rank < 1) {
        tile.suit = PreviousSuit(tile.suit);
        const int rankLimit = TileRankLimit(tile.suit);
        tile.rank = rankLimit;
    }

    LoadTileImage(g_selectedTile);
    UpdateWinningTileFromHand();
    UpdateInvalidTileSelection();
}

void ChangeSelectedTileSuit(int direction)
{
    if (g_selectedTile >= g_hand.size()) {
        return;
    }

    mahjong::Tile& tile = g_hand[g_selectedTile];
    tile.suit = direction > 0 ? NextSuit(tile.suit) : PreviousSuit(tile.suit);
    tile.rank = 1;

    LoadTileImage(g_selectedTile);
    UpdateWinningTileFromHand();
    UpdateInvalidTileSelection();
}

SIZE HandImageSize()
{
    SIZE size{ WindowPadding * 2, WindowPadding * 2 + YakuAreaHeight };
    UINT maxHeight = 0;
    UINT maxWidth = 64;

    for (const TileImage& tileImage : g_tileImages) {
        if (tileImage.image && tileImage.image->GetLastStatus() == Gdiplus::Ok) {
            size.cx += static_cast<LONG>(tileImage.image->GetWidth()) + TileGap;
            maxHeight = std::max(maxHeight, tileImage.image->GetHeight());
            maxWidth = std::max(maxWidth, tileImage.image->GetWidth());
        }
        else {
            size.cx += 64 + TileGap;
            maxHeight = std::max<UINT>(maxHeight, 90);
        }
    }

    if (!g_tileImages.empty()) {
        size.cx -= TileGap;
    }

    if (g_tileImages.size() >= 2) {
        size.cx += TsumoGap;
    }

    size.cy += static_cast<LONG>(maxHeight);

    if (g_context.isRiichi) {
        const LONG candidateWidth = static_cast<LONG>(maxWidth) * MaxWinningCandidateCount
            + CandidateGap * (MaxWinningCandidateCount - 1)
            + CandidateFramePadding * 2;
        size.cx = std::max(size.cx, WindowPadding * 2 + candidateWidth);
        size.cy += CandidateRowGap + static_cast<LONG>(maxHeight) + CandidateFramePadding * 2;
    }

    return size;
}

SIZE TileDrawSize(const TileImage& tileImage)
{
    if (tileImage.image && tileImage.image->GetLastStatus() == Gdiplus::Ok) {
        return {
            static_cast<LONG>(tileImage.image->GetWidth()),
            static_cast<LONG>(tileImage.image->GetHeight())
        };
    }

    return { 64, 90 };
}

LONG HandMaxHeight()
{
    LONG maxHeight = 90;
    for (const TileImage& tileImage : g_tileImages) {
        maxHeight = std::max(maxHeight, TileDrawSize(tileImage).cy);
    }
    return maxHeight;
}

int CandidateTop()
{
    return TileTop() + HandMaxHeight() + CandidateRowGap;
}

int TileLeft(std::size_t index)
{
    int x = WindowPadding;
    for (std::size_t i = 0; i < index && i < g_tileImages.size(); ++i) {
        x += TileDrawSize(g_tileImages[i]).cx + TileGap;
        if (i + 1 == g_tileImages.size() - 1) {
            x += TsumoGap;
        }
    }
    return x;
}

int HandRight()
{
    if (g_tileImages.empty()) {
        return WindowPadding;
    }

    const std::size_t lastIndex = g_tileImages.size() - 1;
    return TileLeft(lastIndex) + TileDrawSize(g_tileImages[lastIndex]).cx;
}

SIZE CandidateImagesSize()
{
    SIZE size{ 0, 0 };
    for (const TileImage& tileImage : g_candidateImages) {
        const SIZE tileSize = TileDrawSize(tileImage);
        size.cx += tileSize.cx + CandidateGap;
        size.cy = std::max(size.cy, tileSize.cy);
    }

    if (!g_candidateImages.empty()) {
        size.cx -= CandidateGap;
    }
    return size;
}

RECT CandidateAreaRect()
{
    const SIZE candidateSize = CandidateImagesSize();
    const int left = WindowPadding;
    const int top = CandidateTop();
    return {
        left - CandidateFramePadding,
        top - CandidateFramePadding,
        left + candidateSize.cx + CandidateFramePadding,
        top + candidateSize.cy + CandidateFramePadding
    };
}

RECT TileInvalidateRect(std::size_t index)
{
    const int x = TileLeft(index);

    SIZE tileSize{ 64, 90 };
    if (index < g_tileImages.size()) {
        tileSize = TileDrawSize(g_tileImages[index]);
    }

    return {
        x - SelectionPadding - SelectionPenWidth,
        TileTop() - SelectionPadding - SelectionPenWidth,
        x + tileSize.cx + SelectionPadding + SelectionPenWidth,
        TileTop() + tileSize.cy + SelectionPadding + SelectionPenWidth
    };
}

void InvalidateTile(HWND hwnd, std::size_t index)
{
    RECT rect = TileInvalidateRect(index);
    InvalidateRect(hwnd, &rect, FALSE);
}

void InvalidateYakuArea(HWND hwnd)
{
    RECT rect{ 0, 0, 4096, TileTop() - SelectionPadding };
    InvalidateRect(hwnd, &rect, FALSE);
}

void InvalidateCandidateArea(HWND hwnd, const RECT& rect)
{
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return;
    }
    InvalidateRect(hwnd, &rect, FALSE);
}

void InvalidateTsumoLabelArea(HWND hwnd)
{
    if (g_tileImages.empty()) {
        return;
    }

    const RECT tileRect = TileImageRect(g_tileImages.size() - 1);
    RECT rect{
        tileRect.left - 44,
        TileTop() - 40,
        tileRect.right + 44,
        tileRect.bottom + SelectionPadding + SelectionPenWidth
    };
    InvalidateRect(hwnd, &rect, FALSE);
}

RECT TileImageRect(std::size_t index)
{
    const int x = TileLeft(index);

    SIZE tileSize{ 64, 90 };
    if (index < g_tileImages.size()) {
        tileSize = TileDrawSize(g_tileImages[index]);
    }

    return {
        x,
        TileTop(),
        x + tileSize.cx,
        TileTop() + tileSize.cy
    };
}

RECT SelectionRect(std::size_t index)
{
    RECT tileRect = TileImageRect(index);
    return {
        tileRect.left - SelectionPadding,
        tileRect.top - SelectionPadding,
        tileRect.right + SelectionPadding,
        tileRect.bottom + SelectionPadding
    };
}

Gdiplus::Rect SelectionDrawRect(std::size_t index)
{
    RECT selectionRect = SelectionRect(index);
    return Gdiplus::Rect(
        static_cast<INT>(selectionRect.left),
        static_cast<INT>(selectionRect.top),
        static_cast<INT>(selectionRect.right - selectionRect.left),
        static_cast<INT>(selectionRect.bottom - selectionRect.top));
}

void DrawSelectionFrame(Gdiplus::Graphics& graphics, std::size_t index)
{
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);

    Gdiplus::Pen selectedPen(
        Gdiplus::Color(255, 255, 218, 72),
        static_cast<Gdiplus::REAL>(SelectionPenWidth));
    Gdiplus::Rect drawRect = SelectionDrawRect(index);

    graphics.DrawRectangle(&selectedPen, drawRect);
}

void DrawTsumoFrame(Gdiplus::Graphics& graphics)
{
}

void DrawTsumoLabel(Gdiplus::Graphics& graphics)
{
    if (g_tileImages.empty()) {
        return;
    }

    const std::size_t tsumoIndex = g_tileImages.size() - 1;
    const RECT tileRect = TileImageRect(tsumoIndex);
    Gdiplus::FontFamily fontFamily(L"Yu Gothic UI");
    Gdiplus::Font font(&fontFamily, 14.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 235, 235));
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);

    Gdiplus::RectF textRect(
        static_cast<Gdiplus::REAL>(tileRect.left - 12),
        static_cast<Gdiplus::REAL>(TileTop() - 32),
        static_cast<Gdiplus::REAL>(tileRect.right - tileRect.left + 24),
        18.0f);

    const wchar_t* label = g_tsumoRonLabelTouched ? (g_isRonTile ? L"ロン" : L"ツモ") : L"ツモ牌";
    graphics.DrawString(label, -1, &font, textRect, &format, &textBrush);
}

void EraseSelectionFrame(Gdiplus::Graphics& graphics, std::size_t index)
{
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);

    Gdiplus::Pen backgroundPen(
        Gdiplus::Color(255, 34, 120, 72),
        static_cast<Gdiplus::REAL>(SelectionPenWidth + 2));
    Gdiplus::Rect drawRect = SelectionDrawRect(index);

    graphics.DrawRectangle(&backgroundPen, drawRect);
}

void MoveSelectionFrame(HWND hwnd, std::size_t oldIndex, std::size_t newIndex)
{
    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    EraseSelectionFrame(graphics, oldIndex);
    DrawSelectionFrame(graphics, newIndex);
    DrawTsumoFrame(graphics);
    ReleaseDC(hwnd, hdc);
}

void ResizeWindowToHand(HWND hwnd)
{
    SIZE clientSize = HandImageSize();
    RECT windowRect{ 0, 0, clientSize.cx, clientSize.cy };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void DrawMissingTile(Gdiplus::Graphics& graphics, int x, int y, const std::wstring& fileName)
{
    Gdiplus::SolidBrush tileBrush(Gdiplus::Color(255, 245, 245, 245));
    Gdiplus::Pen borderPen(Gdiplus::Color(255, 180, 40, 40), 2.0f);
    Gdiplus::FontFamily fontFamily(L"Segoe UI");
    Gdiplus::Font font(&fontFamily, 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 80, 20, 20));
    Gdiplus::Rect rect(x, y, 64, 90);

    graphics.FillRectangle(&tileBrush, rect);
    graphics.DrawRectangle(&borderPen, rect);
    graphics.DrawString(fileName.c_str(), -1, &font, Gdiplus::RectF(
        static_cast<Gdiplus::REAL>(x + 6),
        static_cast<Gdiplus::REAL>(y + 36),
        52.0f,
        24.0f), nullptr, &textBrush);
}

void DrawYakuText(Gdiplus::Graphics& graphics)
{
    Gdiplus::FontFamily fontFamily(L"Yu Gothic UI");
    Gdiplus::Font font(&fontFamily, 18.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font smallFont(&fontFamily, 16.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(
        g_invalidTileSelection
            ? Gdiplus::Color(255, 235, 50, 50)
            : Gdiplus::Color(255, 255, 255, 255));
    Gdiplus::SolidBrush subTextBrush(Gdiplus::Color(255, 230, 245, 255));
    Gdiplus::RectF textRect(
        static_cast<Gdiplus::REAL>(WindowPadding),
        8.0f,
        1600.0f,
        24.0f);
    Gdiplus::RectF shantenRect(
        static_cast<Gdiplus::REAL>(WindowPadding),
        34.0f,
        1600.0f,
        22.0f);

    graphics.DrawString(g_yakuText.c_str(), -1, &font, textRect, nullptr, &textBrush);
    if (!g_shantenText.empty()) {
        graphics.DrawString(g_shantenText.c_str(), -1, &smallFont, shantenRect, nullptr, &subTextBrush);
    }
}

void DrawInvalidTileOverlay(Gdiplus::Graphics& graphics)
{
    if (!g_invalidTileSelection || g_selectedTile >= g_tileImages.size()) {
        return;
    }

    RECT tileRect = TileImageRect(g_selectedTile);
    Gdiplus::SolidBrush overlayBrush(Gdiplus::Color(150, 120, 120, 120));
    Gdiplus::Rect overlayRect(
        static_cast<INT>(tileRect.left),
        static_cast<INT>(tileRect.top),
        static_cast<INT>(tileRect.right - tileRect.left),
        static_cast<INT>(tileRect.bottom - tileRect.top));
    graphics.FillRectangle(&overlayBrush, overlayRect);
}

void DrawWinningCandidates(Gdiplus::Graphics& graphics)
{
    if (g_candidateImages.empty()) {
        return;
    }

    Gdiplus::FontFamily fontFamily(L"Yu Gothic UI");
    Gdiplus::Font font(&fontFamily, 15.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush labelBrush(Gdiplus::Color(255, 220, 235, 255));
    Gdiplus::RectF labelRect(
        static_cast<Gdiplus::REAL>(WindowPadding),
        static_cast<Gdiplus::REAL>(CandidateTop() - 28),
        220.0f,
        20.0f);
    graphics.DrawString(L"和了候補", -1, &font, labelRect, nullptr, &labelBrush);

    RECT frameRect = CandidateAreaRect();
    Gdiplus::Pen framePen(Gdiplus::Color(255, 40, 130, 255), 3.0f);
    Gdiplus::Rect drawFrame(
        static_cast<INT>(frameRect.left),
        static_cast<INT>(frameRect.top),
        static_cast<INT>(frameRect.right - frameRect.left),
        static_cast<INT>(frameRect.bottom - frameRect.top));
    graphics.DrawRectangle(&framePen, drawFrame);

    int x = WindowPadding;
    const int y = CandidateTop();
    for (const TileImage& tileImage : g_candidateImages) {
        const SIZE tileSize = TileDrawSize(tileImage);
        if (tileImage.image && tileImage.image->GetLastStatus() == Gdiplus::Ok) {
            graphics.DrawImage(tileImage.image.get(), x, y);
        }
        else {
            DrawMissingTile(graphics, x, y, tileImage.fileName);
        }
        x += tileSize.cx + CandidateGap;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(hwnd, &paint);
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        Gdiplus::SolidBrush backgroundBrush(Gdiplus::Color(255, 34, 120, 72));
        Gdiplus::Rect paintRect(
            static_cast<INT>(paint.rcPaint.left),
            static_cast<INT>(paint.rcPaint.top),
            static_cast<INT>(paint.rcPaint.right - paint.rcPaint.left),
            static_cast<INT>(paint.rcPaint.bottom - paint.rcPaint.top));
        graphics.FillRectangle(&backgroundBrush, paintRect);
        DrawYakuText(graphics);

        for (std::size_t i = 0; i < g_tileImages.size(); ++i) {
            const TileImage& tileImage = g_tileImages[i];
            const SIZE tileSize = TileDrawSize(tileImage);
            const int x = TileLeft(i);

            if (i == g_selectedTile) {
                DrawSelectionFrame(graphics, i);
            }

            if (tileImage.image && tileImage.image->GetLastStatus() == Gdiplus::Ok) {
                graphics.DrawImage(tileImage.image.get(), x, TileTop());
            }
            else {
                DrawMissingTile(graphics, x, TileTop(), tileImage.fileName);
            }

        }
        DrawTsumoLabel(graphics);
        DrawTsumoFrame(graphics);
        DrawInvalidTileOverlay(graphics);
        DrawWinningCandidates(graphics);

        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT: {
            if (g_invalidTileSelection) {
                return 0;
            }
            const std::size_t oldSelectedTile = g_selectedTile;
            MoveSelectedTile(-1);
            MoveSelectionFrame(hwnd, oldSelectedTile, g_selectedTile);
            return 0;
        }
        case VK_RIGHT: {
            if (g_invalidTileSelection) {
                return 0;
            }
            const std::size_t oldSelectedTile = g_selectedTile;
            MoveSelectedTile(1);
            MoveSelectionFrame(hwnd, oldSelectedTile, g_selectedTile);
            return 0;
        }
        case VK_UP:
        {
            const RECT oldCandidateArea = CandidateAreaRect();
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                ChangeSelectedTileSuit(1);
            }
            else {
                ChangeSelectedTileRank(1);
            }
            UpdateWinningCandidates();
            UpdateYakuText();
            UpdateShantenText();
            InvalidateYakuArea(hwnd);
            InvalidateTile(hwnd, g_selectedTile);
            InvalidateCandidateArea(hwnd, oldCandidateArea);
            InvalidateCandidateArea(hwnd, CandidateAreaRect());
            return 0;
        }
        case VK_DOWN:
        {
            const RECT oldCandidateArea = CandidateAreaRect();
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                ChangeSelectedTileSuit(-1);
            }
            else {
                ChangeSelectedTileRank(-1);
            }
            UpdateWinningCandidates();
            UpdateYakuText();
            UpdateShantenText();
            InvalidateYakuArea(hwnd);
            InvalidateTile(hwnd, g_selectedTile);
            InvalidateCandidateArea(hwnd, oldCandidateArea);
            InvalidateCandidateArea(hwnd, CandidateAreaRect());
            return 0;
        }
        case VK_SPACE:
            if (!g_tileImages.empty() && g_selectedTile == g_tileImages.size() - 1) {
                g_tsumoRonLabelTouched = true;
                g_isRonTile = !g_isRonTile;
                UpdateWinningTileFromHand();
                UpdateWinningCandidates();
                UpdateYakuText();
                UpdateShantenText();
                InvalidateYakuArea(hwnd);
                InvalidateTsumoLabelArea(hwnd);
                InvalidateCandidateArea(hwnd, CandidateAreaRect());
            }
            return 0;
        default:
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int ShowHandWindow(HINSTANCE instance, std::vector<mahjong::Tile> hand, const mahjong::HandContext& context)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
        std::cerr << "Failed to initialize GDI+.\n";
        return 1;
    }

    g_hand = std::move(hand);
    g_context = context;
    g_selectedTile = 0;
    g_isRonTile = false;
    g_tsumoRonLabelTouched = false;
    LoadTileImages(g_hand);
    UpdateWinningTileFromHand();
    UpdateInvalidTileSelection();
    UpdateWinningCandidates();
    UpdateYakuText();
    UpdateShantenText();

    const wchar_t className[] = L"ProjectMahjongHandWindow";
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&windowClass);

    SIZE clientSize = HandImageSize();
    RECT windowRect{ 0, 0, clientSize.cx, clientSize.cy };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"ProjectMahjong - Hand",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    g_tileImages.clear();
    g_candidateImages.clear();
    g_hand.clear();
    g_winningCandidates.clear();
    g_yakuText.clear();
    g_shantenText.clear();
    g_invalidTileSelection = false;
    g_isRonTile = false;
    g_tsumoRonLabelTouched = false;
    Gdiplus::GdiplusShutdown(g_gdiplusToken);
    return static_cast<int>(message.wParam);
}

} // namespace

int main()
{
    using namespace mahjong;

    std::vector<Tile> hand = {
        Man(2), Man(3), Man(4),
        Pin(2), Pin(3), Pin(4),
        Sou(2), Sou(3), Sou(4),
        Man(6), Man(7), Man(8),
        Pin(5), Pin(5)
    };

    HandContext context;
    context.isClosed = true;
    context.isRiichi = true;
    context.isTsumo = true;
    context.winningTile = hand.back();

    const YakuResult result = EvaluateYaku(hand, context);
    if (result.validWinningShape) {
        std::cout << "Yaku:\n";
        for (const Yaku& yaku : result.yaku) {
            std::cout << "- " << yaku.name << " (" << yaku.han << " han)\n";
        }
    }
    else {
        std::cout << "Not a winning hand.\n";
    }

    return ShowHandWindow(GetModuleHandleW(nullptr), hand, context);
}
