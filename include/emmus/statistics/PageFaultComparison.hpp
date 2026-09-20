#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/statistics/PageFaultStatistics.hpp"

namespace emmus::statistics
{

/**
 * @brief Compares page-fault totals and fault rates across replacement policies.
 *
 * Each algorithm is evaluated against the same workload and its page-fault
 * statistics are recorded in a sortable comparison record. The records can be
 * ranked by fault count, inspected by policy, or rendered as a readable summary.
 */
class PageFaultComparison final
{
public:
    using PolicyType =
        emmus::algorithms::replacement::PageReplacementPolicyType;

    struct Result
    {
        PolicyType policy{PolicyType::FIFO};
        PageFaultStatistics statistics{};
        std::size_t totalAccessCount{0U};
        std::size_t pageFaultCount{0U};
        double faultRate{0.0};

        [[nodiscard]] bool operator==(const Result&) const = default;
    };

    PageFaultComparison() = default;

    void recordResult(
        PolicyType policy,
        const PageFaultStatistics& statistics) noexcept;

    [[nodiscard]] const std::vector<Result>& results() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] bool hasPolicy(PolicyType policy) const noexcept;

    [[nodiscard]] std::optional<Result> resultFor(
        PolicyType policy) const noexcept;

    [[nodiscard]] std::string summary() const;

    [[nodiscard]] bool operator==(const PageFaultComparison&) const = default;

private:
    static std::string policyName(PolicyType policy) noexcept;

    void sortResults() noexcept;

    std::vector<Result> results_{};
};

} // namespace emmus::statistics
