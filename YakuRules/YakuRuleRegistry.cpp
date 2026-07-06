#include "YakuRules.h"

namespace mahjong::internal {

std::vector<std::unique_ptr<YakuRule>> CreateYakumanRules()
{
    std::vector<std::unique_ptr<YakuRule>> rules;
    rules.push_back(std::make_unique<TenhoRule>());
    rules.push_back(std::make_unique<ChihoRule>());
    rules.push_back(std::make_unique<KokushiRule>());
    rules.push_back(std::make_unique<SuuankouRule>());
    rules.push_back(std::make_unique<DaisangenRule>());
    rules.push_back(std::make_unique<ShousuushiiRule>());
    rules.push_back(std::make_unique<DaisuushiiRule>());
    rules.push_back(std::make_unique<TsuuiisouRule>());
    rules.push_back(std::make_unique<RyuuiisouRule>());
    rules.push_back(std::make_unique<ChinroutouRule>());
    rules.push_back(std::make_unique<ChuurenPoutouRule>());
    return rules;
}

std::vector<std::unique_ptr<YakuRule>> CreateRegularRules()
{
    std::vector<std::unique_ptr<YakuRule>> rules;
    rules.push_back(std::make_unique<RiichiRule>());
    rules.push_back(std::make_unique<IppatsuRule>());
    rules.push_back(std::make_unique<MenzenTsumoRule>());
    rules.push_back(std::make_unique<RinshanRule>());
    rules.push_back(std::make_unique<ChankanRule>());
    rules.push_back(std::make_unique<HaiteiRule>());
    rules.push_back(std::make_unique<HouteiRule>());
    rules.push_back(std::make_unique<TanyaoRule>());
    rules.push_back(std::make_unique<PinfuRule>());
    rules.push_back(std::make_unique<IipeikouRule>());
    rules.push_back(std::make_unique<YakuhaiRule>());
    rules.push_back(std::make_unique<ToitoiRule>());
    rules.push_back(std::make_unique<SanankouRule>());
    rules.push_back(std::make_unique<ChiitoitsuRule>());
    rules.push_back(std::make_unique<SanshokuDoujunRule>());
    rules.push_back(std::make_unique<SanshokuDoukouRule>());
    rules.push_back(std::make_unique<IttsuRule>());
    rules.push_back(std::make_unique<ChantaRule>());
    rules.push_back(std::make_unique<JunchanRule>());
    rules.push_back(std::make_unique<RyanpeikouRule>());
    rules.push_back(std::make_unique<ShousangenRule>());
    rules.push_back(std::make_unique<HonroutouRule>());
    rules.push_back(std::make_unique<HonitsuRule>());
    rules.push_back(std::make_unique<ChinitsuRule>());
    return rules;
}

std::vector<Yaku> EvaluateRules(
    const std::vector<std::unique_ptr<YakuRule>>& rules,
    const HandAnalysis& analysis,
    const Decomposition* hand,
    const HandContext& context)
{
    std::vector<Yaku> yaku;
    for (const std::unique_ptr<YakuRule>& rule : rules) {
        rule->Evaluate(analysis, hand, context, yaku);
    }
    return yaku;
}

} // namespace mahjong::internal
