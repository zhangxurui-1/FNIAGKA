#include "metric.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

Metric::Metric()
    : event_counter_((size_t)EmitType::kEventNum, 0)
{
    auto real_time_evts_num = static_cast<size_t>(EmitType::kEventNum);
    pending_evs_real_.resize(real_time_evts_num);
    stats_real_.resize(real_time_evts_num);
}

Metric::~Metric()
{
}

void
Metric::Emit(EmitType type, std::string key)
{
    // skip if key already emitted
    if (stats_real_[(size_t)type].find(key) != stats_real_[(size_t)type].end())
    {
        return;
    }

    auto& ev = pending_evs_real_[(size_t)type];
    auto v = ev.find(key);
    if (v == ev.end())
    {
        ev[key] = MetricValueReal{std::chrono::steady_clock::now(), 1};
    }
    else
    {
        v->second.phase_++;
        int ph = v->second.phase_;
        if (ph == TotalPhaseNum.find(type)->second)
        {
            auto now = std::chrono::steady_clock::now();
            auto& s = stats_real_[(size_t)type];
            s[key] = std::chrono::duration_cast<Microseconds>(now - v->second.start_time_);
            pending_evs_real_[(size_t)type].erase(key);
        }
    }
}

void
Metric::Summarize()
{
    std::cout << "Metric Summary:" << std::endl;
    for (size_t i = 0; i < stats_real_.size(); i++)
    {
        auto& st = stats_real_[i];
        std::cout << "\t" << EmitType(i) << ":" << st.size() << " events" << std::endl;
        if (st.empty())
        {
            continue;
        }
        Microseconds total(0);
        for (auto& p : st)
        {
            std::cout << "\t\t" << p.first << ":" << p.second.count() << " us" << std::endl;
            total += p.second;
        }
        std::cout << "\t\tAvg:" << total.count() / st.size() << " us\n\n";
    }
}

std::string
Metric::GenerateStatKey(EmitType type)
{
    std::stringstream ss;
    ss << type << "-" << event_counter_[(size_t)type];
    event_counter_[(size_t)type]++;
    return ss.str();
}

uint
Metric::GetEventCount(EmitType type) const
{
    return event_counter_[(size_t)type];
}
