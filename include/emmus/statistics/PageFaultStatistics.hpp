#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::statistics
{

/**
 * @brief Statistics for page-fault activity across processes and runs.
 *
 * PageFaultStatistics tracks the memory-access workload submitted to the
 * MMU and the subset of accesses that triggered a page fault.  The class
 * maintains both aggregate totals and per-process counters to support
 * rate calculations for individual processes and the simulation as a whole.
 */
class PageFaultStatistics final
{
public:

    using Counter = std::uint64_t;
    using ProcessId = emmus::memory::identifiers::ProcessId;

    struct ProcessStatistics
    {
        Counter accessCount{0U};
        Counter pageFaultCount{0U};

        [[nodiscard]] constexpr bool operator==(const ProcessStatistics&) const = default;
    };

    PageFaultStatistics() noexcept = default;

    PageFaultStatistics(const PageFaultStatistics&) = default;
    PageFaultStatistics(PageFaultStatistics&&) noexcept = default;

    PageFaultStatistics& operator=(const PageFaultStatistics&) = default;
    PageFaultStatistics& operator=(PageFaultStatistics&&) noexcept = default;

    ~PageFaultStatistics() = default;

    /**
     * @brief Records one processed memory access for the supplied process.
     */
    void recordAccess(ProcessId processId) noexcept;

    /**
     * @brief Records one page fault for the supplied process.
     */
    void recordPageFault(ProcessId processId) noexcept;

    /**
     * @brief Returns the total number of memory accesses observed.
     */
    [[nodiscard]] Counter totalAccessCount() const noexcept;

    /**
     * @brief Returns the total number of recorded page faults.
     */
    [[nodiscard]] Counter totalPageFaultCount() const noexcept;

    /**
     * @brief Returns the total number of processes with recorded activity.
     */
    [[nodiscard]] std::size_t processCount() const noexcept;

    /**
     * @brief Returns the total number of accesses emitted by the process.
     */
    [[nodiscard]] Counter accessCount(ProcessId processId) const noexcept;

    /**
     * @brief Returns the number of page faults recorded for the process.
     */
    [[nodiscard]] Counter pageFaultCount(ProcessId processId) const noexcept;

    /**
     * @brief Returns the aggregate page-fault rate across all observed
     * accesses.
     */
    [[nodiscard]] double faultRate() const noexcept;

    /**
     * @brief Returns the page-fault rate for a single process.
     */
    [[nodiscard]] double faultRate(ProcessId processId) const noexcept;

    /**
     * @brief Resets all counters to zero.
     */
    void reset() noexcept;

    [[nodiscard]] bool operator==(const PageFaultStatistics&) const = default;

private:

    std::unordered_map<ProcessId, ProcessStatistics> processStatistics_{};
    Counter totalAccessCount_{0U};
    Counter totalPageFaultCount_{0U};
};

} // namespace emmus::statistics