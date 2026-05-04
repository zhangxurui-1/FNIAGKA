#include <chrono>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

enum class EmitType
{
    kUnknown,
    kComputeSetup,
    kComputePNGen,
    kComputeUserGen,
    kComputeNegotiate,
    kComputeAgree,
    kComputeAddUpd,
    kComputeAddGen,
    kComputeRemove,
    kComputeSplit_2,
    kComputeSplit_3,
    kComputeSplit_4,
    kComputeSplit_5,
    kComputeMergeStandard_2,
    kComputeMergeStandard_3,
    kComputeMergeStandard_4,
    kComputeMergeStandard_5,
    kComputeMergeExtended_2,
    kComputeMergeExtended_3,
    kComputeMergeExtended_4,
    kComputeMergeExtended_5,
    kComputeEncap,
    kComputeDecap,
    kComputeUpdateUpk,
    kComputeUpdateGroupKey,

    kComputeUpdateUpkLaunchV2,
    kComputeUpdateUpkLaunchV2_2,
    kComputeUpdateUpkLaunchV2_3,
    kComputeUpdateUpkLaunchV2_4,
    kComputeUpdateUpkLaunchV2_5,
    kComputeUpdateUpkV2,
    kComputeUpdateGroupKeyV2,

    kEventNum,
};

inline std::ostream&
operator<<(std::ostream& os, EmitType type)
{
    switch (type)
    {
    case EmitType::kUnknown:
        os << "kUnknown";
        break;
    case EmitType::kComputeSetup:
        os << "kComputeSetup";
        break;
    case EmitType::kComputePNGen:
        os << "kComputePNGen";
        break;
    case EmitType::kComputeUserGen:
        os << "kComputeUserGen";
        break;
    case EmitType::kComputeNegotiate:
        os << "kComputeNegotiate";
        break;
    case EmitType::kComputeAgree:
        os << "kComputeAgree";
        break;
    case EmitType::kComputeAddUpd:
        os << "kComputeAddUpd";
        break;
    case EmitType::kComputeAddGen:
        os << "kComputeAddGen";
        break;
    case EmitType::kComputeRemove:
        os << "kComputeRemove";
        break;
    case EmitType::kComputeSplit_2:
        os << "kComputeSplit_2";
        break;
    case EmitType::kComputeSplit_3:
        os << "kComputeSplit_3";
        break;
    case EmitType::kComputeSplit_4:
        os << "kComputeSplit_4";
        break;
    case EmitType::kComputeSplit_5:
        os << "kComputeSplit_5";
        break;
    case EmitType::kComputeMergeStandard_2:
        os << "kComputeMergeStandard_2";
        break;
    case EmitType::kComputeMergeStandard_3:
        os << "kComputeMergeStandard_3";
        break;
    case EmitType::kComputeMergeStandard_4:
        os << "kComputeMergeStandard_4";
        break;
    case EmitType::kComputeMergeStandard_5:
        os << "kComputeMergeStandard_5";
        break;
    case EmitType::kComputeMergeExtended_2:
        os << "kComputeMergeExtended_2";
        break;
    case EmitType::kComputeMergeExtended_3:
        os << "kComputeMergeExtended_3";
        break;
    case EmitType::kComputeMergeExtended_4:
        os << "kComputeMergeExtended_4";
        break;
    case EmitType::kComputeMergeExtended_5:
        os << "kComputeMergeExtended_5";
        break;
    case EmitType::kComputeEncap:
        os << "kComputeEncap";
        break;
    case EmitType::kComputeDecap:
        os << "kComputeDecap";
        break;
    case EmitType::kComputeUpdateUpk:
        os << "kComputeUpdateUpk";
        break;
    case EmitType::kComputeUpdateGroupKey:
        os << "kComputeUpdateGroupKey";
        break;
    case EmitType::kComputeUpdateUpkLaunchV2:
        os << "kComputeUpdateUpkLaunchV2";
        break;
    case EmitType::kComputeUpdateUpkLaunchV2_2:
        os << "kComputeUpdateUpkLaunchV2_2";
        break;
    case EmitType::kComputeUpdateUpkLaunchV2_3:
        os << "kComputeUpdateUpkLaunchV2_3";
        break;
    case EmitType::kComputeUpdateUpkLaunchV2_4:
        os << "kComputeUpdateUpkLaunchV2_4";
        break;
    case EmitType::kComputeUpdateUpkLaunchV2_5:
        os << "kComputeUpdateUpkLaunchV2_5";
        break;
    case EmitType::kComputeUpdateUpkV2:
        os << "kComputeUpdateUpkV2";
        break;
    case EmitType::kComputeUpdateGroupKeyV2:
        os << "kComputeUpdateGroupKeyV2";
        break;
    default:
        os << "Unknown EmitType";
        break;
    }
    return os;
}

const std::unordered_map<EmitType, int> TotalPhaseNum = {
    {EmitType::kComputeSetup, 2},
    {EmitType::kComputePNGen, 2},
    {EmitType::kComputeUserGen, 2},
    {EmitType::kComputeNegotiate, 2},
    {EmitType::kComputeAgree, 2},
    {EmitType::kComputeAddUpd, 2},
    {EmitType::kComputeAddGen, 2},
    {EmitType::kComputeRemove, 2},
    {EmitType::kComputeSplit_2, 2},
    {EmitType::kComputeSplit_3, 2},
    {EmitType::kComputeSplit_4, 2},
    {EmitType::kComputeSplit_5, 2},
    {EmitType::kComputeMergeStandard_2, 2},
    {EmitType::kComputeMergeStandard_3, 2},
    {EmitType::kComputeMergeStandard_4, 2},
    {EmitType::kComputeMergeStandard_5, 2},
    {EmitType::kComputeMergeExtended_2, 2},
    {EmitType::kComputeMergeExtended_3, 2},
    {EmitType::kComputeMergeExtended_4, 2},
    {EmitType::kComputeMergeExtended_5, 2},
    {EmitType::kComputeEncap, 2},
    {EmitType::kComputeDecap, 2},
    {EmitType::kComputeUpdateUpk, 2},
    {EmitType::kComputeUpdateGroupKey, 2},
    {EmitType::kComputeUpdateUpkLaunchV2, 2},
    {EmitType::kComputeUpdateUpkLaunchV2_2, 2},
    {EmitType::kComputeUpdateUpkLaunchV2_3, 2},
    {EmitType::kComputeUpdateUpkLaunchV2_4, 2},
    {EmitType::kComputeUpdateUpkLaunchV2_5, 2},
    {EmitType::kComputeUpdateUpkV2, 2},
    {EmitType::kComputeUpdateGroupKeyV2, 2},
};

class Metric
{
  public:
    using Microseconds = std::chrono::microseconds;

    struct MetricValueReal
    {
        std::chrono::time_point<std::chrono::steady_clock> start_time_;
        int phase_;
    };

    Metric();
    virtual ~Metric();
    void Emit(EmitType type, std::string key);
    void Summarize();

    // Generate a new stat key
    std::string GenerateStatKey(EmitType type);

    uint GetEventCount(EmitType type) const;

  private:
    std::vector<std::map<std::string, MetricValueReal>> pending_evs_real_;
    std::vector<std::map<std::string, Microseconds>> stats_real_;
    std::vector<uint> event_counter_;
};
