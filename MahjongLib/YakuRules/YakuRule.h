#pragma once

#include "../MahjongHandAnalysis.h"

#include <string>
#include <vector>

namespace mahjong::internal {

class YakuRule {
public:
    virtual ~YakuRule() = default;
    virtual void Evaluate(
        const HandAnalysis& analysis,
        const Decomposition* hand,
        const HandContext& context,
        std::vector<Yaku>& yaku) const = 0;

protected:
    inline static constexpr const char* RiichiName = u8"立直";
    inline static constexpr const char* IppatsuName = u8"一発";
    inline static constexpr const char* MenzenTsumoName = u8"門前清自摸和";
    inline static constexpr const char* RinshanName = u8"嶺上開花";
    inline static constexpr const char* ChankanName = u8"槍槓";
    inline static constexpr const char* HaiteiName = u8"海底撈月";
    inline static constexpr const char* HouteiName = u8"河底撈魚";
    inline static constexpr const char* TanyaoName = u8"断么九";
    inline static constexpr const char* PinfuName = u8"平和";
    inline static constexpr const char* IipeikouName = u8"一盃口";
    inline static constexpr const char* YakuhaiName = u8"役牌";
    inline static constexpr const char* ToitoiName = u8"対々和";
    inline static constexpr const char* SanankouName = u8"三暗刻";
    inline static constexpr const char* ChiitoitsuName = u8"七対子";
    inline static constexpr const char* SanshokuDoujunName = u8"三色同順";
    inline static constexpr const char* SanshokuDoukouName = u8"三色同刻";
    inline static constexpr const char* IttsuName = u8"一気通貫";
    inline static constexpr const char* ChantaName = u8"混全帯么九";
    inline static constexpr const char* JunchanName = u8"純全帯么九";
    inline static constexpr const char* RyanpeikouName = u8"二盃口";
    inline static constexpr const char* ShousangenName = u8"小三元";
    inline static constexpr const char* HonroutouName = u8"混老頭";
    inline static constexpr const char* HonitsuName = u8"混一色";
    inline static constexpr const char* ChinitsuName = u8"清一色";
    inline static constexpr const char* TenhoName = u8"天和";
    inline static constexpr const char* ChihoName = u8"地和";
    inline static constexpr const char* KokushiName = u8"国士無双";
    inline static constexpr const char* SuuankouName = u8"四暗刻";
    inline static constexpr const char* DaisangenName = u8"大三元";
    inline static constexpr const char* ShousuushiiName = u8"小四喜";
    inline static constexpr const char* DaisuushiiName = u8"大四喜";
    inline static constexpr const char* TsuuiisouName = u8"字一色";
    inline static constexpr const char* RyuuiisouName = u8"緑一色";
    inline static constexpr const char* ChinroutouName = u8"清老頭";
    inline static constexpr const char* ChuurenPoutouName = u8"九蓮宝燈";

    static void Add(std::vector<Yaku>& yaku, const std::string& name, int han);
    static void AddYakuman(std::vector<Yaku>& yaku, const std::string& name);
    static bool IsHonor(int tile);
    static bool IsTerminal(int tile);
    static bool IsTerminalOrHonor(int tile);
    static bool IsDragon(int tile);
    static bool IsWindTile(int tile, Wind wind);
    static bool IsFlushFamily(const HandAnalysis& analysis, bool allowHonors);
    static bool AllMeldsAre(const Decomposition& hand, Meld::Type type);
    static int CountTriplets(const Decomposition& hand);
    static bool HasSequence(const Decomposition& hand, int tile);
};

} // namespace mahjong::internal
