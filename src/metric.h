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
    kComputeSplit,
    kComputeMergeStandard,
    kComputeMergeExtended,
    kComputeEncap,
    kComputeDecap,

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
    case EmitType::kComputeSplit:
        os << "kComputeSplit";
        break;
    case EmitType::kComputeMergeStandard:
        os << "kComputeMergeStandard";
        break;
    case EmitType::kComputeMergeExtended:
        os << "kComputeMergeExtended";
        break;
    case EmitType::kComputeEncap:
        os << "kComputeEncap";
        break;
    case EmitType::kComputeDecap:
        os << "kComputeDecap";
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
    {EmitType::kComputeSplit, 2},
    {EmitType::kComputeMergeStandard, 2},
    {EmitType::kComputeMergeExtended, 2},
    {EmitType::kComputeEncap, 2},
    {EmitType::kComputeDecap, 2},
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

  private:
    std::vector<std::map<std::string, MetricValueReal>> pending_evs_real_;
    std::vector<std::map<std::string, Microseconds>> stats_real_;
    std::vector<uint> event_counter_;
};
