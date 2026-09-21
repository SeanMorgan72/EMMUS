#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/statistics/PageReplacementStatistics.hpp"

namespace emmus::statistics
{

/**
 * @brief Compares execution-time overhead across replacement policies.
 *
 * Each algorithm is evaluated against the same workload and its replacement
 * statistics are stored with the corresponding total simulation time. Results can
 * be ranked by average replacement time, summarized for reporting, and inspected
 * by policy.
 */
class ExecutionTimeComparison final
{
public:
    using PolicyType =
        emmus::algorithms::replacement::PageReplacementPolicyType;

    struct Result
    {
        PolicyType policy{PolicyType::FIFO};
        PageReplacementStatistics statistics{};
        std::chrono::nanoseconds totalSimulationTime{0};

        [[nodiscard]] std::chrono::nanoseconds totalReplacementTime() const noexcept
        {
            return statistics.totalExecutionTime();
        }

        [[nodiscard]] double averageReplacementTimeMilliseconds() const noexcept
        {
            return statistics.averageExecutionTimeMilliseconds();
        }

        [[nodiscard]] double simulationTimeMilliseconds() const noexcept
        {
            return std::chrono::duration<double, std::milli>(
                totalSimulationTime).count();
        }

        [[nodiscard]] bool operator==(const Result&) const = default;
    };

    ExecutionTimeComparison() = default;

    void recordResult(
        PolicyType policy,
        const PageReplacementStatistics& statistics,
        std::chrono::nanoseconds totalSimulationTime) noexcept;

    [[nodiscard]] const std::vector<Result>& results() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] bool hasPolicy(PolicyType policy) const noexcept;

    [[nodiscard]] std::optional<Result> resultFor(
        PolicyType policy) const noexcept;

    [[nodiscard]] std::string summary() const;

    [[nodiscard]] bool operator==(const ExecutionTimeComparison&) const = default;

private:
    static std::string policyName(PolicyType policy) noexcept;

    void sortResults() noexcept;

    std::vector<Result> results_{};
};

} // namespace emmus::statistics
