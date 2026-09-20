#include "emmus/statistics/PageFaultComparison.hpp"

#include <algorithm>
#include <sstream>

namespace emmus::statistics
{

void PageFaultComparison::recordResult(
    PolicyType policy,
    const PageFaultStatistics& statistics) noexcept
{
    Result result;
    result.policy = policy;
    result.statistics = statistics;
    result.totalAccessCount = static_cast<std::size_t>(
        statistics.totalAccessCount());
    result.pageFaultCount = static_cast<std::size_t>(
        statistics.totalPageFaultCount());
    result.faultRate = statistics.faultRate();

    results_.push_back(result);
    sortResults();
}

const std::vector<PageFaultComparison::Result>&
PageFaultComparison::results() const noexcept
{
    return results_;
}

std::size_t PageFaultComparison::size() const noexcept
{
    return results_.size();
}

bool PageFaultComparison::empty() const noexcept
{
    return results_.empty();
}

bool PageFaultComparison::hasPolicy(PolicyType policy) const noexcept
{
    return resultFor(policy).has_value();
}

std::optional<PageFaultComparison::Result>
PageFaultComparison::resultFor(PolicyType policy) const noexcept
{
    for (const auto& result : results_)
    {
        if (result.policy == policy)
        {
            return result;
        }
    }

    return std::nullopt;
}

std::string PageFaultComparison::summary() const
{
    if (results_.empty())
    {
        return "Page-fault comparison: no algorithm results recorded.";
    }

    std::ostringstream stream;
    stream << "Page-fault comparison:\n";

    for (const auto& result : results_)
    {
        stream << "  - " << policyName(result.policy)
               << ": " << result.pageFaultCount
               << " faults / " << result.totalAccessCount
               << " accesses (fault rate "
               << result.faultRate << ")\n";
    }

    return stream.str();
}

std::string PageFaultComparison::policyName(PolicyType policy) noexcept
{
    switch (policy)
    {
        case PolicyType::FIFO:
            return "FIFO";
        case PolicyType::LRU:
            return "LRU";
        case PolicyType::CLOCK:
            return "CLOCK";
        case PolicyType::OPTIMAL:
            return "OPTIMAL";
    }

    return "UNKNOWN";
}

void PageFaultComparison::sortResults() noexcept
{
    std::sort(
        results_.begin(),
        results_.end(),
        [](const Result& left, const Result& right)
        {
            if (left.pageFaultCount != right.pageFaultCount)
            {
                return left.pageFaultCount < right.pageFaultCount;
            }

            return static_cast<std::uint8_t>(left.policy) <
                   static_cast<std::uint8_t>(right.policy);
        });
}

} // namespace emmus::statistics
