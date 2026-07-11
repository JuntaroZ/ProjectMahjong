#include "MahjongYaku.h"

#include <algorithm>
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
#include <windowsx.h>
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
constexpr int YakuAreaHeight = 270;
constexpr int SelectionPadding = 6;
constexpr int SelectionPenWidth = 3;
constexpr int SortButtonWidth = 124;
constexpr int TsumoRonButtonWidth = 150;
constexpr int MeldButtonWidth = 72;
constexpr int SortButtonHeight = 28;
constexpr int ButtonGap = 10;
constexpr int MeldAreaGap = 34;

struct TileImage {
    std::wstring fileName;
    std::unique_ptr<Gdiplus::Image> image;
};

enum class SelectionArea {
    HandTile,
    OpenMeld
};

enum class OpenMeldType {
    Chi,
    Pon,
    Kan
};

struct OpenMeld {
    OpenMeldType type;
    std::vector<mahjong::Tile> tiles;
};

struct OpenMeldImages {
    std::vector<TileImage> tiles;
};

struct MeldButtonState {
    bool canChi{ false };
    bool canPon{ false };
    bool canKan{ false };
    bool canReturn{ false };
};

std::vector<TileImage> g_tileImages;
std::vector<TileImage> g_candidateImages;
std::vector<OpenMeldImages> g_openMeldImages;
std::vector<mahjong::Tile> g_hand;
std::vector<OpenMeld> g_openMelds;
std::vector<mahjong::Tile> g_winningCandidates;
mahjong::HandContext g_context;
mahjong::HandContext g_baseContext;
std::wstring g_yakuText;
std::wstring g_shantenText;
MeldButtonState g_meldButtons;
bool g_invalidTileSelection = false;
bool g_isRonTile = false;
bool g_tsumoRonLabelTouched = false;
std::filesystem::path g_imageDirectory;
SelectionArea g_selectionArea = SelectionArea::HandTile;
std::size_t g_selectedTile = 0;
std::size_t g_selectedMeld = 0;
ULONG_PTR g_gdiplusToken = 0;

std::vector<std::wstring> CollectDisplayYakuLines();
int TileSortIndex(const mahjong::Tile& tile);
RECT TileImageRect(std::size_t index);
SIZE TileDrawSize(const TileImage& tileImage);
int HandRight();
RECT SortButtonRect();
RECT TsumoRonButtonRect();
RECT ChiButtonRect();
RECT PonButtonRect();
RECT KanButtonRect();
RECT ReturnMeldButtonRect();
void ResizeWindowToHand(HWND hwnd);

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

std::vector<mahjong::Tile> AnalysisHand()
{
    std::vector<mahjong::Tile> tiles;
    if (!g_hand.empty()) {
        tiles.insert(tiles.end(), g_hand.begin(), g_hand.end() - 1);
    }
    for (const OpenMeld& meld : g_openMelds) {
        tiles.insert(tiles.end(), meld.tiles.begin(), meld.tiles.end());
    }
    if (!g_hand.empty()) {
        tiles.push_back(g_hand.back());
    }
    return tiles;
}

mahjong::HandState CurrentHandState()
{
    mahjong::HandState state;
    state.closedTiles = g_hand;
    state.context = g_context;
    state.openMelds.reserve(g_openMelds.size());

    for (const OpenMeld& meld : g_openMelds) {
        mahjong::OpenMeld openMeld;
        switch (meld.type) {
        case OpenMeldType::Chi:
            openMeld.type = mahjong::MeldType::Sequence;
            break;
        case OpenMeldType::Pon:
            openMeld.type = mahjong::MeldType::Triplet;
            break;
        case OpenMeldType::Kan:
            openMeld.type = mahjong::MeldType::Quad;
            break;
        }
        openMeld.tiles = meld.tiles;
        state.openMelds.push_back(std::move(openMeld));
    }

    return state;
}

std::vector<mahjong::Tile> AllTilesForCurrentState()
{
    std::vector<mahjong::Tile> tiles = g_hand;
    for (const OpenMeld& meld : g_openMelds) {
        tiles.insert(tiles.end(), meld.tiles.begin(), meld.tiles.end());
    }
    return tiles;
}

bool SameTile(const mahjong::Tile& left, const mahjong::Tile& right)
{
    return left == right;
}

int CountTileInHand(const mahjong::Tile& target)
{
    return static_cast<int>(std::count(g_hand.begin(), g_hand.end(), target));
}

bool IsNumberTile(const mahjong::Tile& tile)
{
    return tile.suit != mahjong::Suit::Honor;
}

mahjong::Tile MakeTile(mahjong::Suit suit, int rank)
{
    return { suit, rank };
}

std::vector<mahjong::Tile> SortedMeldTiles(std::vector<mahjong::Tile> tiles)
{
    std::sort(tiles.begin(), tiles.end(), [](const mahjong::Tile& left, const mahjong::Tile& right) {
        return TileSortIndex(left) < TileSortIndex(right);
    });
    return tiles;
}

void UpdateYakuText()
{
    if (g_invalidTileSelection) {
        g_yakuText = L"4個以上は選べません";
        return;
    }

    try {
        const mahjong::HandViewAnalysis analysis = mahjong::AnalyzeHandView(CurrentHandState());
        if (!analysis.winning && !analysis.ready) {
            g_yakuText.clear();
            return;
        }

        const std::vector<std::wstring> yakuLines = CollectDisplayYakuLines();

        if (yakuLines.empty()) {
            g_yakuText = L"成立役: なし";
            return;
        }

        g_yakuText = L"成立役:";
        for (const std::wstring& line : yakuLines) {
            g_yakuText += L"\n";
            g_yakuText += line;
        }
    }
    catch (...) {
        g_yakuText.clear();
    }
}

void UpdateShantenText()
{
    if (g_invalidTileSelection) {
        g_shantenText.clear();
        return;
    }

    try {
        const mahjong::HandViewAnalysis analysis = mahjong::AnalyzeHandView(CurrentHandState());
        if (analysis.winning) {
            g_shantenText = analysis.riichiOnlyWin ? L"和了（リーチのみ）" : L"和了";
        }
        else if (analysis.ready) {
            g_shantenText = L"テンパイ";
        }
        else if (analysis.shanten == 1) {
            g_shantenText = L"イーシャンテン";
        }
        else if (analysis.shanten == 2) {
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

void UpdateInvalidTileSelection()
{
    g_invalidTileSelection = false;
    if (g_selectionArea == SelectionArea::HandTile && g_selectedTile < g_hand.size()) {
        g_invalidTileSelection = mahjong::CountTile(AllTilesForCurrentState(), g_hand[g_selectedTile]) > 4;
    }
}

void UpdateWinningTileFromHand()
{
    if (!g_hand.empty()) {
        g_context.winningTile = g_hand.back();
    }
    g_context.isTsumo = !g_isRonTile;
    g_context.isClosed = g_openMelds.empty() && g_baseContext.isClosed;
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

int TileSortIndex(const mahjong::Tile& tile)
{
    int suitIndex = 0;
    switch (tile.suit) {
    case mahjong::Suit::Man:
        suitIndex = 0;
        break;
    case mahjong::Suit::Pin:
        suitIndex = 1;
        break;
    case mahjong::Suit::Sou:
        suitIndex = 2;
        break;
    case mahjong::Suit::Honor:
        suitIndex = 3;
        break;
    }

    return suitIndex * 10 + tile.rank;
}

void SortHand()
{
    const auto sortEnd = g_hand.size() >= 2 ? g_hand.end() - 1 : g_hand.end();
    std::sort(g_hand.begin(), sortEnd, [](const mahjong::Tile& left, const mahjong::Tile& right) {
        return TileSortIndex(left) < TileSortIndex(right);
    });
    if (!g_hand.empty()) {
        g_selectedTile = std::min(g_selectedTile, g_hand.size() - 1);
    }
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

void LoadOpenMeldImages()
{
    g_openMeldImages.clear();
    g_openMeldImages.reserve(g_openMelds.size());

    for (const OpenMeld& meld : g_openMelds) {
        OpenMeldImages images;
        images.tiles.reserve(meld.tiles.size());
        for (const mahjong::Tile& tile : meld.tiles) {
            const std::wstring fileName = TileImageFileName(tile);
            auto image = std::make_unique<Gdiplus::Image>((g_imageDirectory / fileName).c_str());
            images.tiles.push_back({ fileName, std::move(image) });
        }
        g_openMeldImages.push_back(std::move(images));
    }
}

bool CanChiSelectedTile()
{
    if (g_selectionArea != SelectionArea::HandTile || g_selectedTile >= g_hand.size()) {
        return false;
    }
    if (g_hand.size() >= 2 && g_selectedTile == g_hand.size() - 1) {
        return false;
    }

    const mahjong::Tile selected = g_hand[g_selectedTile];
    if (!IsNumberTile(selected)) {
        return false;
    }

    for (int start = selected.rank; start >= selected.rank - 2; --start) {
        if (start < 1 || start + 2 > 9) {
            continue;
        }

        std::vector<mahjong::Tile> needed = {
            MakeTile(selected.suit, start),
            MakeTile(selected.suit, start + 1),
            MakeTile(selected.suit, start + 2)
        };
        bool possible = true;
        for (const mahjong::Tile& tile : needed) {
            if (CountTileInHand(tile) < 1) {
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

bool CanPonSelectedTile()
{
    return g_selectionArea == SelectionArea::HandTile
        && g_selectedTile < g_hand.size()
        && !(g_hand.size() >= 2 && g_selectedTile == g_hand.size() - 1)
        && CountTileInHand(g_hand[g_selectedTile]) >= 3;
}

bool CanKanSelectedTile()
{
    return g_selectionArea == SelectionArea::HandTile
        && g_selectedTile < g_hand.size()
        && !(g_hand.size() >= 2 && g_selectedTile == g_hand.size() - 1)
        && CountTileInHand(g_hand[g_selectedTile]) >= 4;
}

void UpdateMeldButtonState()
{
    g_meldButtons.canChi = CanChiSelectedTile();
    g_meldButtons.canPon = CanPonSelectedTile();
    g_meldButtons.canKan = CanKanSelectedTile();
    g_meldButtons.canReturn = g_selectionArea == SelectionArea::OpenMeld && g_selectedMeld < g_openMelds.size();
}

void UpdateWinningCandidates()
{
    g_winningCandidates.clear();
    const mahjong::HandState handState = CurrentHandState();
    if (g_invalidTileSelection || !g_context.isRiichi || AllTilesForCurrentState().size() != 14) {
        LoadWinningCandidateImages();
        return;
    }

    g_winningCandidates = mahjong::FindWinningCandidates(handState);

    LoadWinningCandidateImages();
}

std::vector<std::wstring> CollectDisplayYakuLines()
{
    std::vector<mahjong::Yaku> yaku = mahjong::EvaluateDisplayYaku(CurrentHandState(), g_winningCandidates);
    const std::vector<mahjong::YakuScore> scores = mahjong::CalculateDisplayYakuScores(yaku, g_context);

    std::vector<std::wstring> lines;
    lines.reserve(scores.size());
    for (const mahjong::YakuScore& score : scores) {
        std::wstring line = Utf8ToWide(score.yaku.name);
        line += L"（";
        line += Utf8ToWide(score.text);
        line += L"）";
        lines.push_back(std::move(line));
    }
    return lines;
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
    const int handCount = static_cast<int>(g_hand.size());
    const int meldCount = static_cast<int>(g_openMelds.size());
    const int totalCount = handCount + meldCount;
    if (totalCount == 0) {
        return;
    }

    int current = 0;
    if (g_selectionArea == SelectionArea::HandTile) {
        current = static_cast<int>(std::min(g_selectedTile, g_hand.empty() ? 0 : g_hand.size() - 1));
    }
    else {
        current = handCount + static_cast<int>(std::min(g_selectedMeld, g_openMelds.empty() ? 0 : g_openMelds.size() - 1));
    }

    const int next = (current + totalCount + direction) % totalCount;
    if (next < handCount) {
        g_selectionArea = SelectionArea::HandTile;
        g_selectedTile = static_cast<std::size_t>(next);
    }
    else {
        g_selectionArea = SelectionArea::OpenMeld;
        g_selectedMeld = static_cast<std::size_t>(next - handCount);
    }
    UpdateInvalidTileSelection();
    UpdateMeldButtonState();
}

void ChangeSelectedTileRank(int direction)
{
    if (g_selectionArea != SelectionArea::HandTile || g_selectedTile >= g_hand.size()) {
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
    UpdateMeldButtonState();
}

void ChangeSelectedTileSuit(int direction)
{
    if (g_selectionArea != SelectionArea::HandTile || g_selectedTile >= g_hand.size()) {
        return;
    }

    mahjong::Tile& tile = g_hand[g_selectedTile];
    tile.suit = direction > 0 ? NextSuit(tile.suit) : PreviousSuit(tile.suit);
    tile.rank = 1;

    LoadTileImage(g_selectedTile);
    UpdateWinningTileFromHand();
    UpdateInvalidTileSelection();
    UpdateMeldButtonState();
}

void RemoveTilesForMeld(const std::vector<mahjong::Tile>& tiles)
{
    std::vector<bool> remove(g_hand.size(), false);
    for (const mahjong::Tile& tile : tiles) {
        for (std::size_t i = 0; i < g_hand.size(); ++i) {
            if (!remove[i] && SameTile(g_hand[i], tile)) {
                remove[i] = true;
                break;
            }
        }
    }

    std::vector<mahjong::Tile> remaining;
    remaining.reserve(g_hand.size());
    for (std::size_t i = 0; i < g_hand.size(); ++i) {
        if (!remove[i]) {
            remaining.push_back(g_hand[i]);
        }
    }
    g_hand = std::move(remaining);
}

bool MakeChiMeld()
{
    if (!CanChiSelectedTile()) {
        return false;
    }

    const mahjong::Tile selected = g_hand[g_selectedTile];
    for (int start = selected.rank - 2; start <= selected.rank; ++start) {
        if (start < 1 || start + 2 > 9) {
            continue;
        }

        std::vector<mahjong::Tile> tiles = {
            MakeTile(selected.suit, start),
            MakeTile(selected.suit, start + 1),
            MakeTile(selected.suit, start + 2)
        };

        bool possible = true;
        for (const mahjong::Tile& tile : tiles) {
            if (CountTileInHand(tile) < 1) {
                possible = false;
                break;
            }
        }

        if (possible) {
            g_openMelds.push_back({ OpenMeldType::Chi, tiles });
            RemoveTilesForMeld(tiles);
            g_selectionArea = SelectionArea::OpenMeld;
            g_selectedMeld = g_openMelds.empty() ? 0 : g_openMelds.size() - 1;
            return true;
        }
    }

    return false;
}

bool MakeSameTileMeld(OpenMeldType type, int count)
{
    if (g_selectionArea != SelectionArea::HandTile || g_selectedTile >= g_hand.size()) {
        return false;
    }
    const mahjong::Tile selected = g_hand[g_selectedTile];
    if (CountTileInHand(selected) < count) {
        return false;
    }

    std::vector<mahjong::Tile> tiles(static_cast<std::size_t>(count), selected);
    g_openMelds.push_back({ type, tiles });
    RemoveTilesForMeld(tiles);
    g_selectionArea = SelectionArea::OpenMeld;
    g_selectedMeld = g_openMelds.empty() ? 0 : g_openMelds.size() - 1;
    return true;
}

bool ReturnSelectedMeld()
{
    if (g_selectionArea != SelectionArea::OpenMeld || g_selectedMeld >= g_openMelds.size()) {
        return false;
    }

    const auto insertPosition = g_hand.empty() ? g_hand.end() : g_hand.end() - 1;
    g_hand.insert(insertPosition, g_openMelds[g_selectedMeld].tiles.begin(), g_openMelds[g_selectedMeld].tiles.end());
    g_openMelds.erase(g_openMelds.begin() + static_cast<std::ptrdiff_t>(g_selectedMeld));
    g_selectionArea = SelectionArea::HandTile;
    g_selectedTile = g_hand.size() >= 2 ? g_hand.size() - 2 : 0;
    g_selectedMeld = 0;
    return true;
}

void RefreshAfterHandStructureChanged(HWND hwnd)
{
    LoadTileImages(g_hand);
    LoadOpenMeldImages();
    UpdateWinningTileFromHand();
    UpdateInvalidTileSelection();
    UpdateMeldButtonState();
    UpdateWinningCandidates();
    UpdateYakuText();
    UpdateShantenText();
    ResizeWindowToHand(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

SIZE MeldImagesSize(const OpenMeldImages& meldImages)
{
    SIZE size{ 0, 0 };
    for (const TileImage& tileImage : meldImages.tiles) {
        const SIZE tileSize = TileDrawSize(tileImage);
        size.cx += tileSize.cx + CandidateGap;
        size.cy = std::max(size.cy, tileSize.cy);
    }
    if (!meldImages.tiles.empty()) {
        size.cx -= CandidateGap;
    }
    return size;
}

int OpenMeldLeft(std::size_t index)
{
    int x = HandRight() + MeldAreaGap;
    for (std::size_t i = 0; i < index && i < g_openMeldImages.size(); ++i) {
        x += MeldImagesSize(g_openMeldImages[i]).cx + MeldAreaGap;
    }
    return x;
}

RECT OpenMeldRect(std::size_t index)
{
    if (index >= g_openMeldImages.size()) {
        return { 0, 0, 0, 0 };
    }
    const SIZE size = MeldImagesSize(g_openMeldImages[index]);
    const int left = OpenMeldLeft(index);
    return {
        left,
        TileTop(),
        left + size.cx,
        TileTop() + size.cy
    };
}

int OpenMeldsRight()
{
    if (g_openMeldImages.empty()) {
        return HandRight();
    }
    const RECT rect = OpenMeldRect(g_openMeldImages.size() - 1);
    return rect.right;
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

    if (!g_openMeldImages.empty()) {
        size.cx = std::max<LONG>(size.cx, OpenMeldsRight() + WindowPadding);
    }
    size.cx = std::max<LONG>(size.cx, ReturnMeldButtonRect().right + WindowPadding);

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

RECT CandidateInvalidateRect()
{
    LONG tileWidth = 64;
    LONG tileHeight = 90;
    for (const TileImage& tileImage : g_tileImages) {
        const SIZE tileSize = TileDrawSize(tileImage);
        tileWidth = std::max(tileWidth, tileSize.cx);
        tileHeight = std::max(tileHeight, tileSize.cy);
    }
    for (const TileImage& tileImage : g_candidateImages) {
        const SIZE tileSize = TileDrawSize(tileImage);
        tileWidth = std::max(tileWidth, tileSize.cx);
        tileHeight = std::max(tileHeight, tileSize.cy);
    }

    const LONG candidateWidth = tileWidth * MaxWinningCandidateCount
        + CandidateGap * (MaxWinningCandidateCount - 1);
    const int top = CandidateTop();
    return {
        WindowPadding - CandidateFramePadding - SelectionPenWidth,
        top - 32,
        WindowPadding + candidateWidth + CandidateFramePadding + SelectionPenWidth,
        top + tileHeight + CandidateFramePadding + SelectionPenWidth
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

RECT OpenMeldInvalidateRect(std::size_t index)
{
    RECT meldRect = OpenMeldRect(index);
    if (meldRect.right <= meldRect.left || meldRect.bottom <= meldRect.top) {
        return meldRect;
    }

    return {
        meldRect.left - SelectionPadding - SelectionPenWidth,
        meldRect.top - SelectionPadding - SelectionPenWidth,
        meldRect.right + SelectionPadding + SelectionPenWidth,
        meldRect.bottom + SelectionPadding + SelectionPenWidth
    };
}

RECT SelectionInvalidateRect(SelectionArea area, std::size_t index)
{
    return area == SelectionArea::HandTile
        ? TileInvalidateRect(index)
        : OpenMeldInvalidateRect(index);
}

void InvalidateSelectionArea(HWND hwnd, SelectionArea area, std::size_t index)
{
    RECT rect = SelectionInvalidateRect(area, index);
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return;
    }
    InvalidateRect(hwnd, &rect, FALSE);
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

void InvalidateYakuTextArea(HWND hwnd)
{
    RECT rect{ 0, 0, 4096, 210 };
    InvalidateRect(hwnd, &rect, FALSE);
}

void InvalidateCommandButtonArea(HWND hwnd)
{
    const RECT sortRect = SortButtonRect();
    const RECT returnRect = ReturnMeldButtonRect();
    RECT rect{
        sortRect.left - SelectionPenWidth,
        sortRect.top - SelectionPenWidth,
        returnRect.right + SelectionPenWidth,
        returnRect.bottom + SelectionPenWidth
    };
    InvalidateRect(hwnd, &rect, FALSE);
}

void InvalidateMeldButtonArea(HWND hwnd)
{
    const RECT chiRect = ChiButtonRect();
    const RECT returnRect = ReturnMeldButtonRect();
    RECT rect{
        chiRect.left - SelectionPenWidth,
        chiRect.top - SelectionPenWidth,
        returnRect.right + SelectionPenWidth,
        returnRect.bottom + SelectionPenWidth
    };
    InvalidateRect(hwnd, &rect, FALSE);
}

RECT SortButtonRect()
{
    const int left = WindowPadding;
    const int top = 214;
    return {
        left,
        top,
        left + SortButtonWidth,
        top + SortButtonHeight
    };
}

RECT TsumoRonButtonRect()
{
    const RECT sortRect = SortButtonRect();
    const int left = sortRect.right + 10;
    const int top = sortRect.top;
    return {
        left,
        top,
        left + TsumoRonButtonWidth,
        top + SortButtonHeight
    };
}

RECT ButtonRectAfter(const RECT& previous, int width)
{
    const int left = previous.right + ButtonGap;
    return {
        left,
        previous.top,
        left + width,
        previous.bottom
    };
}

RECT ChiButtonRect()
{
    return ButtonRectAfter(TsumoRonButtonRect(), MeldButtonWidth);
}

RECT PonButtonRect()
{
    return ButtonRectAfter(ChiButtonRect(), MeldButtonWidth);
}

RECT KanButtonRect()
{
    return ButtonRectAfter(PonButtonRect(), MeldButtonWidth);
}

RECT ReturnMeldButtonRect()
{
    return ButtonRectAfter(KanButtonRect(), MeldButtonWidth);
}

bool IsPointInRect(const RECT& rect, int x, int y)
{
    return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
}

void InvalidateCandidateArea(HWND hwnd, const RECT& rect)
{
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return;
    }
    InvalidateRect(hwnd, &rect, FALSE);
}

void InvalidateWinningCandidatesArea(HWND hwnd)
{
    const RECT rect = CandidateInvalidateRect();
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

Gdiplus::Rect OpenMeldSelectionDrawRect(std::size_t index)
{
    RECT meldRect = OpenMeldRect(index);
    return Gdiplus::Rect(
        static_cast<INT>(meldRect.left - SelectionPadding),
        static_cast<INT>(meldRect.top - SelectionPadding),
        static_cast<INT>(meldRect.right - meldRect.left + SelectionPadding * 2),
        static_cast<INT>(meldRect.bottom - meldRect.top + SelectionPadding * 2));
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

void DrawOpenMeldSelectionFrame(Gdiplus::Graphics& graphics, std::size_t index)
{
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    Gdiplus::Pen selectedPen(
        Gdiplus::Color(255, 255, 218, 72),
        static_cast<Gdiplus::REAL>(SelectionPenWidth));
    graphics.DrawRectangle(&selectedPen, OpenMeldSelectionDrawRect(index));
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
    Gdiplus::Font font(&fontFamily, 15.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
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
        176.0f);
    Gdiplus::RectF shantenRect(
        static_cast<Gdiplus::REAL>(WindowPadding),
        186.0f,
        1600.0f,
        22.0f);

    graphics.DrawString(g_yakuText.c_str(), -1, &font, textRect, nullptr, &textBrush);
    if (!g_shantenText.empty()) {
        graphics.DrawString(g_shantenText.c_str(), -1, &smallFont, shantenRect, nullptr, &subTextBrush);
    }
}

void DrawSortButton(Gdiplus::Graphics& graphics)
{
    RECT rect = SortButtonRect();
    Gdiplus::Rect buttonRect(
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top);

    Gdiplus::SolidBrush buttonBrush(Gdiplus::Color(255, 238, 244, 240));
    Gdiplus::Pen borderPen(Gdiplus::Color(255, 34, 86, 58), 2.0f);
    graphics.FillRectangle(&buttonBrush, buttonRect);
    graphics.DrawRectangle(&borderPen, buttonRect);

    Gdiplus::FontFamily fontFamily(L"Yu Gothic UI");
    Gdiplus::Font font(&fontFamily, 14.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 20, 60, 40));
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    graphics.DrawString(L"配牌ソート", -1, &font, Gdiplus::RectF(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top)), &format, &textBrush);
}

void DrawTsumoRonButton(Gdiplus::Graphics& graphics)
{
    RECT rect = TsumoRonButtonRect();
    Gdiplus::Rect buttonRect(
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top);

    Gdiplus::SolidBrush buttonBrush(Gdiplus::Color(255, 238, 244, 240));
    Gdiplus::Pen borderPen(Gdiplus::Color(255, 34, 86, 58), 2.0f);
    graphics.FillRectangle(&buttonBrush, buttonRect);
    graphics.DrawRectangle(&borderPen, buttonRect);

    Gdiplus::FontFamily fontFamily(L"Yu Gothic UI");
    Gdiplus::Font font(&fontFamily, 14.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 20, 60, 40));
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    const std::wstring label = L"ツモ／ロン切替";
    graphics.DrawString(label.c_str(), -1, &font, Gdiplus::RectF(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top)), &format, &textBrush);
}

void DrawCommandButton(Gdiplus::Graphics& graphics, const RECT& rect, const wchar_t* label, bool enabled)
{
    Gdiplus::Rect buttonRect(
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top);

    Gdiplus::SolidBrush buttonBrush(enabled
        ? Gdiplus::Color(255, 238, 244, 240)
        : Gdiplus::Color(255, 185, 195, 188));
    Gdiplus::Pen borderPen(enabled
        ? Gdiplus::Color(255, 34, 86, 58)
        : Gdiplus::Color(255, 115, 130, 120), 2.0f);
    graphics.FillRectangle(&buttonBrush, buttonRect);
    graphics.DrawRectangle(&borderPen, buttonRect);

    Gdiplus::FontFamily fontFamily(L"Yu Gothic UI");
    Gdiplus::Font font(&fontFamily, 14.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(enabled
        ? Gdiplus::Color(255, 20, 60, 40)
        : Gdiplus::Color(255, 95, 105, 100));
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    graphics.DrawString(label, -1, &font, Gdiplus::RectF(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top)), &format, &textBrush);
}

void DrawMeldButtons(Gdiplus::Graphics& graphics)
{
    DrawCommandButton(graphics, ChiButtonRect(), L"チー", g_meldButtons.canChi);
    DrawCommandButton(graphics, PonButtonRect(), L"ポン", g_meldButtons.canPon);
    DrawCommandButton(graphics, KanButtonRect(), L"カン", g_meldButtons.canKan);
    DrawCommandButton(graphics, ReturnMeldButtonRect(), L"戻す", g_meldButtons.canReturn);
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

void DrawOpenMelds(Gdiplus::Graphics& graphics)
{
    for (std::size_t meldIndex = 0; meldIndex < g_openMeldImages.size(); ++meldIndex) {
        int x = OpenMeldLeft(meldIndex);
        const int y = TileTop();
        for (const TileImage& tileImage : g_openMeldImages[meldIndex].tiles) {
            const SIZE tileSize = TileDrawSize(tileImage);
            if (tileImage.image && tileImage.image->GetLastStatus() == Gdiplus::Ok) {
                graphics.DrawImage(tileImage.image.get(), x, y);
            }
            else {
                DrawMissingTile(graphics, x, y, tileImage.fileName);
            }
            x += tileSize.cx + CandidateGap;
        }

        if (g_selectionArea == SelectionArea::OpenMeld && meldIndex == g_selectedMeld) {
            DrawOpenMeldSelectionFrame(graphics, meldIndex);
        }
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
        DrawSortButton(graphics);
        DrawTsumoRonButton(graphics);
        DrawMeldButtons(graphics);

        for (std::size_t i = 0; i < g_tileImages.size(); ++i) {
            const TileImage& tileImage = g_tileImages[i];
            const SIZE tileSize = TileDrawSize(tileImage);
            const int x = TileLeft(i);

            if (g_selectionArea == SelectionArea::HandTile && i == g_selectedTile) {
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
        DrawOpenMelds(graphics);
        DrawWinningCandidates(graphics);

        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT: {
            if (g_invalidTileSelection && g_selectionArea == SelectionArea::HandTile) {
                return 0;
            }
            const SelectionArea oldSelectionArea = g_selectionArea;
            const std::size_t oldSelectionIndex = g_selectionArea == SelectionArea::HandTile ? g_selectedTile : g_selectedMeld;
            MoveSelectedTile(-1);
            const std::size_t newSelectionIndex = g_selectionArea == SelectionArea::HandTile ? g_selectedTile : g_selectedMeld;
            InvalidateSelectionArea(hwnd, oldSelectionArea, oldSelectionIndex);
            InvalidateSelectionArea(hwnd, g_selectionArea, newSelectionIndex);
            InvalidateMeldButtonArea(hwnd);
            return 0;
        }
        case VK_RIGHT: {
            if (g_invalidTileSelection && g_selectionArea == SelectionArea::HandTile) {
                return 0;
            }
            const SelectionArea oldSelectionArea = g_selectionArea;
            const std::size_t oldSelectionIndex = g_selectionArea == SelectionArea::HandTile ? g_selectedTile : g_selectedMeld;
            MoveSelectedTile(1);
            const std::size_t newSelectionIndex = g_selectionArea == SelectionArea::HandTile ? g_selectedTile : g_selectedMeld;
            InvalidateSelectionArea(hwnd, oldSelectionArea, oldSelectionIndex);
            InvalidateSelectionArea(hwnd, g_selectionArea, newSelectionIndex);
            InvalidateMeldButtonArea(hwnd);
            return 0;
        }
        case VK_UP:
        {
            if (g_selectionArea != SelectionArea::HandTile) {
                return 0;
            }
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                ChangeSelectedTileSuit(1);
            }
            else {
                ChangeSelectedTileRank(1);
            }
            UpdateWinningCandidates();
            UpdateYakuText();
            UpdateShantenText();
            InvalidateYakuTextArea(hwnd);
            InvalidateCommandButtonArea(hwnd);
            InvalidateTile(hwnd, g_selectedTile);
            InvalidateWinningCandidatesArea(hwnd);
            return 0;
        }
        case VK_DOWN:
        {
            if (g_selectionArea != SelectionArea::HandTile) {
                return 0;
            }
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                ChangeSelectedTileSuit(-1);
            }
            else {
                ChangeSelectedTileRank(-1);
            }
            UpdateWinningCandidates();
            UpdateYakuText();
            UpdateShantenText();
            InvalidateYakuTextArea(hwnd);
            InvalidateCommandButtonArea(hwnd);
            InvalidateTile(hwnd, g_selectedTile);
            InvalidateWinningCandidatesArea(hwnd);
            return 0;
        }
        default:
            break;
        }
        break;
    case WM_LBUTTONDOWN: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        if (IsPointInRect(SortButtonRect(), x, y)) {
            SortHand();
            LoadTileImages(g_hand);
            UpdateWinningTileFromHand();
            UpdateInvalidTileSelection();
            UpdateMeldButtonState();
            UpdateWinningCandidates();
            UpdateYakuText();
            UpdateShantenText();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (IsPointInRect(TsumoRonButtonRect(), x, y)) {
            g_tsumoRonLabelTouched = true;
            g_isRonTile = !g_isRonTile;
            UpdateWinningTileFromHand();
            UpdateMeldButtonState();
            UpdateWinningCandidates();
            UpdateYakuText();
            UpdateShantenText();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (IsPointInRect(ChiButtonRect(), x, y)) {
            if (g_meldButtons.canChi && MakeChiMeld()) {
                RefreshAfterHandStructureChanged(hwnd);
            }
            return 0;
        }
        if (IsPointInRect(PonButtonRect(), x, y)) {
            if (g_meldButtons.canPon && MakeSameTileMeld(OpenMeldType::Pon, 3)) {
                RefreshAfterHandStructureChanged(hwnd);
            }
            return 0;
        }
        if (IsPointInRect(KanButtonRect(), x, y)) {
            if (g_meldButtons.canKan && MakeSameTileMeld(OpenMeldType::Kan, 4)) {
                RefreshAfterHandStructureChanged(hwnd);
            }
            return 0;
        }
        if (IsPointInRect(ReturnMeldButtonRect(), x, y)) {
            if (g_meldButtons.canReturn && ReturnSelectedMeld()) {
                RefreshAfterHandStructureChanged(hwnd);
            }
            return 0;
        }
        break;
    }
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
    g_baseContext = context;
    g_openMelds.clear();
    g_openMeldImages.clear();
    g_selectionArea = SelectionArea::HandTile;
    g_selectedTile = 0;
    g_selectedMeld = 0;
    g_isRonTile = false;
    g_tsumoRonLabelTouched = false;
    LoadTileImages(g_hand);
    LoadOpenMeldImages();
    UpdateWinningTileFromHand();
    UpdateInvalidTileSelection();
    UpdateMeldButtonState();
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
    g_openMeldImages.clear();
    g_hand.clear();
    g_openMelds.clear();
    g_winningCandidates.clear();
    g_yakuText.clear();
    g_shantenText.clear();
    g_meldButtons = {};
    g_invalidTileSelection = false;
    g_isRonTile = false;
    g_tsumoRonLabelTouched = false;
    g_selectionArea = SelectionArea::HandTile;
    g_selectedMeld = 0;
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
