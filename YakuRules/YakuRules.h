#pragma once

#include "YakuRule.h"

#include <memory>

namespace mahjong::internal {

class RiichiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class IppatsuRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class MenzenTsumoRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class RinshanRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ChankanRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class HaiteiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class HouteiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class TanyaoRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class PinfuRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class IipeikouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class YakuhaiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ToitoiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class SanankouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ChiitoitsuRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class SanshokuDoujunRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class IttsuRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class HonitsuRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ChinitsuRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class TenhoRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ChihoRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class KokushiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class SuuankouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class DaisangenRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ShousuushiiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class DaisuushiiRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class TsuuiisouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class RyuuiisouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ChinroutouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ChuurenPoutouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class SanshokuDoukouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ChantaRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class JunchanRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class RyanpeikouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class ShousangenRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

class HonroutouRule final : public YakuRule {
public:
    void Evaluate(const HandAnalysis&, const Decomposition*, const HandContext&, std::vector<Yaku>&) const override;
};

std::vector<std::unique_ptr<YakuRule>> CreateYakumanRules();
std::vector<std::unique_ptr<YakuRule>> CreateRegularRules();
std::vector<Yaku> EvaluateRules(
    const std::vector<std::unique_ptr<YakuRule>>& rules,
    const HandAnalysis& analysis,
    const Decomposition* hand,
    const HandContext& context);

} // namespace mahjong::internal
