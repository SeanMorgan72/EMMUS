#include "emmus/statistics/ExecutionTimeComparison.hpp"

#include <algorithm>
#include <sstream>

namespace emmus::statistics
{

void ExecutionTimeComparison::recordResult(
    PolicyType policy,
    const PageReplacementStatistics& statistics,
    std::chrono::nanoseconds totalSimulationTime) noexcept
{
    Result result;
    result.policy = policy;
    result.statistics = statistics;
    result.totalSimulationTime = totalSimulationTime;

    results_.push_back(result);
    sortResults();
}

const std::vector<ExecutionTimeComparison::Result>&
ExecutionTimeComparison::results() const noexcept
{
    return results_;
}

std::size_t ExecutionTimeComparison::size() const noexcept
{
    return results_.size();
}

bool ExecutionTimeComparison::empty() const noexcept
{
    return results_.empty();
}

bool ExecutionTimeComparison::hasPolicy(PolicyType policy) const noexcept
{
    return resultFor(policy).has_value();
}

std::optional<ExecutionTimeComparison::Result>
ExecutionTimeComparison::resultFor(PolicyType policy) const noexcept
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

std::string ExecutionTimeComparison::summary() const
{
    if (results_.empty())
    {
        return "Execution time comparison: no algorithm results recorded.";
    }

    std::ostringstream stream;
    stream << "Execution time comparison:\n";

    for (const auto& result : results_)
    {
        stream << "  - " << policyName(result.policy)
               << ": average replacement time "
               << result.averageReplacementTimeMilliseconds()
               << " ms, total simulation time "
               << result.simulationTimeMilliseconds()
               << " ms\n";
    }

    return stream.str();
}

std::string ExecutionTimeComparison::policyName(PolicyType policy) noexcept
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

void ExecutionTimeComparison::sortResults() noexcept
{
    std::sort(
        results_.begin(),
        results_.end(),
        [](const Result& left, const Result& right)
        {
            const auto leftAverage = left.averageReplacementTimeMilliseconds();
            const auto rightAverage = right.averageReplacementTimeMilliseconds();

            if (leftAverage != rightAverage)
            {
                return leftAverage < rightAverage;
            }

            return static_cast<std::uint8_t>(left.policy) <
                   static_cast<std::uint8_t>(right.policy);
        });
}

} // namespace emmus::statistics
