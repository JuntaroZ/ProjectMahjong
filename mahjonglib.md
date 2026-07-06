# MahjongLib 仕様書

## 概要

MahjongLib は、麻雀の手牌解析、和了形判定、役判定、シャンテン計算、和了候補探索、表示用点数計算を行う C++17 ライブラリです。

本ライブラリは `mahjong` 名前空間に公開 API を持ち、画面表示や入力操作に依存しません。Win32 / GDI+ を使用する画面処理は `ProjectMahjong.cpp` 側の責務です。

## 対象ファイル

- `MahjongYaku.h`
  - 外部公開 API
  - 牌、手牌条件、役、解析結果の型定義
- `MahjongYaku.cpp`
  - 役判定 API
  - シャンテン計算
  - 和了候補探索
  - 表示用役一覧生成
  - 表示用点数計算
- `MahjongHandAnalysis.h` / `MahjongHandAnalysis.cpp`
  - 手牌の内部解析
  - 面子分解、七対子、国士無双の形判定
- `YakuRules`
  - 役ごとの判定クラス
  - 役名、翻数、役満設定

## 牌表現

### Suit

```cpp
enum class Suit {
    Man,
    Pin,
    Sou,
    Honor
};
```

| 値 | 意味 |
| --- | --- |
| `Man` | 萬子 |
| `Pin` | 筒子 |
| `Sou` | 索子 |
| `Honor` | 字牌 |

### Tile

```cpp
struct Tile {
    Suit suit;
    int rank;
};
```

`rank` の有効範囲は以下です。

| 種類 | 範囲 |
| --- | --- |
| 萬子 | 1 - 9 |
| 筒子 | 1 - 9 |
| 索子 | 1 - 9 |
| 字牌 | 1 - 7 |

字牌の並びは以下です。

| rank | 牌 |
| --- | --- |
| 1 | 東 |
| 2 | 南 |
| 3 | 西 |
| 4 | 北 |
| 5 | 白 |
| 6 | 發 |
| 7 | 中 |

## 牌生成 API

```cpp
Tile Man(int rank);
Tile Pin(int rank);
Tile Sou(int rank);
Tile Honor(int rank);
```

例:

```cpp
std::vector<mahjong::Tile> hand = {
    mahjong::Man(1), mahjong::Man(2), mahjong::Man(3),
    mahjong::Pin(1), mahjong::Pin(2), mahjong::Pin(3),
    mahjong::Sou(1), mahjong::Sou(2), mahjong::Sou(3),
    mahjong::Man(7), mahjong::Man(8), mahjong::Man(9),
    mahjong::Honor(1), mahjong::Honor(1)
};
```

## 和了条件

### Wind

```cpp
enum class Wind {
    East = 1,
    South = 2,
    West = 3,
    North = 4
};
```

### HandContext

```cpp
struct HandContext {
    Tile winningTile;
    Wind seatWind;
    Wind roundWind;
    bool isClosed;
    bool isTsumo;
    bool isRiichi;
    bool isIppatsu;
    bool isRinshan;
    bool isChankan;
    bool isHaitei;
    bool isHoutei;
    bool isTenho;
    bool isChiho;
};
```

| フィールド | 意味 |
| --- | --- |
| `winningTile` | 和了牌。平和の待ち判定、三暗刻のロン判定などで使用します。 |
| `seatWind` | 自風。役牌、平和の雀頭判定で使用します。 |
| `roundWind` | 場風。役牌、平和の雀頭判定で使用します。 |
| `isClosed` | 門前かどうか。立直、一発、門前清自摸和、一盃口、食い下がり役などで使用します。 |
| `isTsumo` | ツモ和了かどうか。門前清自摸和、平和ツモ、三暗刻の判定で使用します。 |
| `isRiichi` | 立直状態かどうか。立直、一発、テンパイ時表示用判定で使用します。 |
| `isIppatsu` | 一発かどうか。`isRiichi` と併用します。 |
| `isRinshan` | 嶺上開花かどうか。 |
| `isChankan` | 槍槓かどうか。 |
| `isHaitei` | 海底撈月かどうか。 |
| `isHoutei` | 河底撈魚かどうか。 |
| `isTenho` | 天和かどうか。 |
| `isChiho` | 地和かどうか。 |

## 役関連の型

### Yaku

```cpp
struct Yaku {
    std::string name;
    int han;
    bool yakuman;
};
```

| フィールド | 意味 |
| --- | --- |
| `name` | UTF-8 の役名 |
| `han` | 翻数。役満は現在 `13` として扱います。 |
| `yakuman` | 役満の場合 `true` |

### YakuResult

```cpp
struct YakuResult {
    bool validWinningShape;
    std::vector<Yaku> yaku;
};
```

| フィールド | 意味 |
| --- | --- |
| `validWinningShape` | 和了形として成立しているか |
| `yaku` | 成立役一覧 |

### YakuScore

```cpp
struct YakuScore {
    Yaku yaku;
    int points;
    std::string text;
};
```

| フィールド | 意味 |
| --- | --- |
| `yaku` | 対象役 |
| `points` | 表示用点数 |
| `text` | UTF-8 の表示用文字列。例: `1翻 / 1000点` |

## 手牌表示向け解析結果

### HandViewAnalysis

```cpp
struct HandViewAnalysis {
    bool invalidTileCount;
    bool winning;
    bool ready;
    bool riichiOnlyWin;
    int shanten;
    std::vector<Yaku> yaku;
    std::vector<Tile> winningCandidates;
};
```

| フィールド | 意味 |
| --- | --- |
| `invalidTileCount` | 同じ牌が5枚以上ある場合 `true` |
| `winning` | 現在の14牌が和了形の場合 `true` |
| `ready` | 和了候補が存在する場合 `true` |
| `riichiOnlyWin` | 和了形だが表示対象役が無い場合 `true` |
| `shanten` | シャンテン数。和了形は負数になります。 |
| `yaku` | 表示用成立役一覧 |
| `winningCandidates` | 和了候補牌一覧 |

## 公開 API

### EvaluateYaku

```cpp
YakuResult EvaluateYaku(const std::vector<Tile>& closedTiles, const HandContext& context);
```

14牌の和了形判定と役判定を行います。

- 入力は必ず14牌です。
- 14牌以外の場合は `std::invalid_argument` を送出します。
- 和了形でない場合、`validWinningShape == false` を返します。
- 和了形の場合、役満を先に評価します。
- 役満が成立した場合、通常役は評価結果に含めません。
- 通常役は、複数の面子分解候補の中から合計翻数が最大になるものを採用します。

### EvaluateCurrentYaku

```cpp
YakuResult EvaluateCurrentYaku(const std::vector<Tile>& closedTiles, const HandContext& context);
```

現在の14牌に対して、和了形でなくても成立し得る表示用役を評価します。

- 入力は必ず14牌です。
- 14牌以外の場合は `std::invalid_argument` を送出します。
- `validWinningShape` は和了形かどうかを示します。
- 手牌画面で「現在成立している役」を表示する用途で使用します。

### CalculateShanten

```cpp
int CalculateShanten(const std::vector<Tile>& tiles);
```

シャンテン数を計算します。

対応している形:

- 通常形
- 七対子
- 国士無双

3種類のうち最小のシャンテン数を返します。

### FindWinningCandidates

```cpp
std::vector<Tile> FindWinningCandidates(const std::vector<Tile>& hand, const HandContext& context);
```

14牌のうち、最後の1牌をツモ／ロン牌として扱い、残り13牌に対する和了候補を列挙します。

仕様:

- `hand.size() != 14` の場合は空配列を返します。
- 同一牌が5枚以上ある場合は空配列を返します。
- 13牌に対して全34種類の牌を試します。
- すでに4枚使われている牌は候補に含めません。
- `EvaluateYaku` で和了形になる牌を候補として返します。

### EvaluateDisplayYaku

```cpp
std::vector<Yaku> EvaluateDisplayYaku(
    const std::vector<Tile>& hand,
    const HandContext& context,
    const std::vector<Tile>& winningCandidates);
```

画面表示向けの役一覧を生成します。

仕様:

- 現在の14牌が和了形の場合、その和了形の役を返します。
- 表示用役一覧から `立直` は除外します。
- 立直状態で和了候補がある場合、13牌 + 各和了候補牌で役判定を行い、候補ごとの役を統合します。
- 最後に現在の14牌に対して `EvaluateCurrentYaku` を行い、現在成立している役を追加します。
- 同じ役名は重複して追加しません。

### CalculateDisplayYakuScores

```cpp
std::vector<YakuScore> CalculateDisplayYakuScores(
    const std::vector<Yaku>& yaku,
    const HandContext& context);
```

表示用の点数文字列を生成します。

現在の点数仕様:

- 表示は `翻数 / 点数` 形式です。
- 親、子、ロン、ツモなどの支払い表記は表示しません。
- 点数は子ロン相当の表示点として計算します。
- 100点単位に切り上げます。
- 符計算は簡易仕様です。

簡易符:

| 条件 | 符 |
| --- | --- |
| 七対子 | 25符 |
| 平和ツモ | 20符 |
| その他 | 30符 |

満貫以上:

| 条件 | 点数 |
| --- | --- |
| 満貫 | 8000点 |
| 跳満 | 12000点 |
| 倍満 | 16000点 |
| 三倍満 | 24000点 |
| 役満 | 32000点 |

### AnalyzeHandView

```cpp
HandViewAnalysis AnalyzeHandView(
    const std::vector<Tile>& hand,
    const HandContext& context);
```

手牌画面用の総合解析を行います。

実行内容:

- 同一牌5枚以上の検出
- シャンテン数計算
- 現在の14牌の和了形判定
- 立直状態での和了候補探索
- 表示用役一覧生成
- 和了形だが表示対象役が無い場合の `riichiOnlyWin` 判定

### AllTileKinds

```cpp
std::vector<Tile> AllTileKinds();
```

34種類すべての牌を返します。

順序:

1. 萬子 1 - 9
2. 筒子 1 - 9
3. 索子 1 - 9
4. 字牌 1 - 7

### CountTile

```cpp
int CountTile(const std::vector<Tile>& tiles, const Tile& target);
```

指定した牌が手牌内に何枚あるかを返します。

### HasInvalidTileCount

```cpp
bool HasInvalidTileCount(const std::vector<Tile>& tiles);
```

同一牌が5枚以上含まれている場合 `true` を返します。

### TileToString

```cpp
std::string TileToString(const Tile& tile);
```

牌を文字列に変換します。

例:

| 牌 | 戻り値 |
| --- | --- |
| 萬子1 | `1m` |
| 筒子9 | `9p` |
| 索子5 | `5s` |
| 字牌7 | `7z` |

## 対応役

### 役満

| 役名 | 翻数 | 条件 |
| --- | --- | --- |
| 天和 | 13 | `context.isTenho == true` |
| 地和 | 13 | `context.isChiho == true` |
| 国士無双 | 13 | 国士無双形 |

### 通常役

| 役名 | 翻数 | 主な条件 |
| --- | --- | --- |
| 立直 | 1 | `isRiichi && isClosed` |
| 一発 | 1 | `isIppatsu && isRiichi` |
| 門前清自摸和 | 1 | `isTsumo && isClosed` |
| 嶺上開花 | 1 | `isRinshan` |
| 槍槓 | 1 | `isChankan` |
| 海底撈月 | 1 | `isHaitei` |
| 河底撈魚 | 1 | `isHoutei` |
| 断么九 | 1 | 么九牌を含まない |
| 平和 | 1 | 門前、全順子、役牌雀頭なし、両面待ち |
| 一盃口 | 1 | 門前、同一順子2組 |
| 役牌 | 1 | 三元牌、自風、場風の刻子 |
| 対々和 | 2 | 全刻子 |
| 三暗刻 | 2 | 暗刻3組。ロンで和了牌が刻子を完成させる場合、その刻子は暗刻扱いしません。 |
| 七対子 | 2 | 七対子形 |
| 三色同順 | 門前2 / 鳴き1 | 萬子、筒子、索子で同じ数字の順子 |
| 一気通貫 | 門前2 / 鳴き1 | 同一色で 123、456、789 |
| 混一色 | 門前3 / 鳴き2 | 1色 + 字牌 |
| 清一色 | 門前6 / 鳴き5 | 1色のみ |

## 平和の待ち判定

平和は以下の場合に成立しません。

- 待ちが雀頭の場合
- 嵌張待ちの場合
- 辺張待ちの場合
- 雀頭が三元牌、自風、場風の場合
- 門前でない場合

現在の実装では `context.winningTile` を使用して両面待ちかどうかを判定します。

## 三暗刻のロン判定

三暗刻は `context.isTsumo` により判定が変わります。

- ツモの場合、刻子は暗刻として扱います。
- ロンの場合、和了牌で完成した刻子は明刻扱いとして暗刻数から除外します。

## 内部解析仕様

内部では牌を 0 - 33 のインデックスで扱います。

| 範囲 | 牌 |
| --- | --- |
| 0 - 8 | 萬子 |
| 9 - 17 | 筒子 |
| 18 - 26 | 索子 |
| 27 - 33 | 字牌 |

`AnalyzeHand` は以下を解析します。

- 牌カウント
- 通常形の全分解候補
- 七対子形
- 国士無双形

同一牌が5枚以上ある場合は `std::invalid_argument` を送出します。

## 制限事項

- 現在の公開APIは基本的に閉じた手牌ベースです。
- 鳴き面子を外部から明示的に渡す API はありません。
- ドラ、赤ドラ、裏ドラ、場棒、本場は未対応です。
- 点数計算の符は簡易仕様です。
- ダブル役満、数え役満の細かい扱いは未整備です。
- 役判定は現在実装済みの役に限定されます。

## 使用例

```cpp
#include "MahjongYaku.h"

#include <iostream>
#include <vector>

int main()
{
    using namespace mahjong;

    std::vector<Tile> hand = {
        Man(1), Man(2), Man(3),
        Pin(1), Pin(2), Pin(3),
        Sou(1), Sou(2), Sou(3),
        Man(7), Man(8), Man(9),
        Honor(1), Honor(1)
    };

    HandContext context;
    context.winningTile = hand.back();
    context.seatWind = Wind::East;
    context.roundWind = Wind::East;
    context.isClosed = true;
    context.isTsumo = false;

    YakuResult result = EvaluateYaku(hand, context);
    if (result.validWinningShape) {
        std::vector<YakuScore> scores = CalculateDisplayYakuScores(result.yaku, context);
        for (const YakuScore& score : scores) {
            std::cout << score.yaku.name << " " << score.text << "\n";
        }
    }
}
```
