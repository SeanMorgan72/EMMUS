#include "SimulationStatisticsVisualizationView.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

#include "emmus/simulation/Simulation.hpp"
#include "emmus/simulation/SimulationResult.hpp"

namespace emmus::gui
{
namespace
{

std::string policyName(
    emmus::algorithms::replacement::PageReplacementPolicyType policy)
{
    using Policy = emmus::algorithms::replacement::PageReplacementPolicyType;
    switch (policy)
    {
        case Policy::FIFO: return "FIFO";
        case Policy::LRU: return "LRU";
        case Policy::CLOCK: return "CLOCK";
        case Policy::OPTIMAL: return "OPTIMAL";
    }
    return "UNKNOWN";
}

double pageFaultRatePercent(
    const SimulationStatisticsVisualizationView::Snapshot& snapshot) noexcept
{
    if (snapshot.memoryAccessCount == 0U)
    {
        return 0.0;
    }
    return 100.0 * static_cast<double>(snapshot.pageFaultCount) /
           static_cast<double>(snapshot.memoryAccessCount);
}

double milliseconds(std::chrono::nanoseconds duration) noexcept
{
    return std::chrono::duration<double, std::milli>(duration).count();
}

std::string counterDelta(std::uint64_t current, std::uint64_t baseline)
{
    if (current >= baseline)
    {
        return "+" + std::to_string(current - baseline);
    }
    return "-" + std::to_string(baseline - current);
}

void appendComparisonRow(
    std::ostringstream& stream,
    const std::string& label,
    std::uint64_t current,
    std::uint64_t baseline)
{
    stream << label << ": " << baseline << " -> " << current << " ("
           << counterDelta(current, baseline) << ")\n";
}

} // namespace

void SimulationStatisticsVisualizationView::update(
    const emmus::simulation::SimulationProgress& progress)
{
    const auto& statistics = progress.executionStatistics;
    snapshot_.replacementPolicy = policyName(progress.replacementPolicy);
    snapshot_.status = Status::Running;
    snapshot_.memoryAccessCount = statistics.memoryAccessCount();
    snapshot_.totalAccessCount = progress.totalAccessCount;
    snapshot_.successfulAccessCount = statistics.successfulAccessCount();
    snapshot_.failedAccessCount = statistics.failedAccessCount();
    snapshot_.pageFaultCount = statistics.pageFaultCount();
    snapshot_.pageReplacementCount = statistics.pageReplacementCount();
    snapshot_.dirtyEvictionCount = statistics.dirtyEvictionCount();
    snapshot_.executionTime = progress.elapsedTime;
    snapshot_.replacementExecutionTime =
        progress.pageReplacementStatistics.totalExecutionTime();
    snapshot_.memory = progress.physicalMemoryUtilization;
    hasSnapshot_ = true;
}

void SimulationStatisticsVisualizationView::update(
    const emmus::simulation::SimulationResult& result,
    const Utilization& memory)
{
    const auto& statistics = result.executionResult().statistics();
    snapshot_.replacementPolicy = policyName(
        result.configuration().replacementPolicy());
    snapshot_.status = result.succeeded() ? Status::Completed : Status::Failed;
    snapshot_.memoryAccessCount = statistics.memoryAccessCount();
    snapshot_.totalAccessCount = statistics.memoryAccessCount();
    snapshot_.successfulAccessCount = statistics.successfulAccessCount();
    snapshot_.failedAccessCount = statistics.failedAccessCount();
    snapshot_.pageFaultCount = statistics.pageFaultCount();
    snapshot_.pageReplacementCount = statistics.pageReplacementCount();
    snapshot_.dirtyEvictionCount = statistics.dirtyEvictionCount();
    snapshot_.executionTime = result.executionTime();
    snapshot_.replacementExecutionTime =
        result.pageReplacementStatistics().totalExecutionTime();
    snapshot_.memory = memory;
    hasSnapshot_ = true;
}

void SimulationStatisticsVisualizationView::setComparison(Snapshot baseline)
{
    comparison_ = std::move(baseline);
}

void SimulationStatisticsVisualizationView::clearComparison() noexcept
{
    comparison_.reset();
}

void SimulationStatisticsVisualizationView::reset() noexcept
{
    snapshot_ = {};
    comparison_.reset();
    hasSnapshot_ = false;
}

bool SimulationStatisticsVisualizationView::hasSnapshot() const noexcept
{
    return hasSnapshot_;
}

const SimulationStatisticsVisualizationView::Snapshot&
SimulationStatisticsVisualizationView::snapshot() const noexcept
{
    return snapshot_;
}

const std::optional<SimulationStatisticsVisualizationView::Snapshot>&
SimulationStatisticsVisualizationView::comparison() const noexcept
{
    return comparison_;
}

std::string SimulationStatisticsVisualizationView::render() const
{
    std::ostringstream stream;
    stream << "Simulation Statistics\n";
    if (!hasSnapshot_)
    {
        stream << "[no statistics available]\n";
        return stream.str();
    }

    const char* status = "RUNNING";
    if (snapshot_.status == Status::Completed)
    {
        status = "COMPLETED";
    }
    else if (snapshot_.status == Status::Failed)
    {
        status = "FAILED";
    }

    const auto utilization = std::isfinite(snapshot_.memory.utilizationPercent)
        ? snapshot_.memory.utilizationPercent
        : 0.0;
    const auto barPercent = std::clamp(utilization, 0.0, 100.0);
    const auto filled = static_cast<std::size_t>(barPercent / 10.0);

    stream << "Status: " << status << "\n";
    stream << "Replacement policy: " << snapshot_.replacementPolicy << "\n";
    stream << "Memory accesses: " << snapshot_.memoryAccessCount << "/"
           << snapshot_.totalAccessCount << " ("
           << snapshot_.successfulAccessCount << " successful, "
           << snapshot_.failedAccessCount << " failed)\n";
    stream << "Page faults: " << snapshot_.pageFaultCount << " ("
           << std::fixed << std::setprecision(1)
           << pageFaultRatePercent(snapshot_) << "% fault rate)\n";
    stream << "Page replacements: " << snapshot_.pageReplacementCount << "\n";
    stream << "Dirty-page evictions: " << snapshot_.dirtyEvictionCount << "\n";
    stream << "Physical memory: " << snapshot_.memory.allocatedFrames << "/"
           << snapshot_.memory.totalFrames << " frames allocated; "
           << snapshot_.memory.freeFrames << " free\n";
    stream << "Memory utilization: " << std::setprecision(1) << utilization
           << "% [";
    for (std::size_t index = 0U; index < 10U; ++index)
    {
        stream << (index < filled ? '#' : '-');
    }
    stream << "]\n";
    stream << "Execution time: " << std::setprecision(3)
           << milliseconds(snapshot_.executionTime) << " ms\n";
    stream << "Replacement time: "
           << milliseconds(snapshot_.replacementExecutionTime) << " ms\n";

    if (comparison_.has_value())
    {
        const auto& baseline = *comparison_;
        stream << "\nComparison: " << baseline.replacementPolicy << " -> "
               << snapshot_.replacementPolicy << "\n";
        appendComparisonRow(
            stream, "Memory accesses", snapshot_.memoryAccessCount,
            baseline.memoryAccessCount);
        appendComparisonRow(
            stream, "Successful accesses", snapshot_.successfulAccessCount,
            baseline.successfulAccessCount);
        appendComparisonRow(
            stream, "Failed accesses", snapshot_.failedAccessCount,
            baseline.failedAccessCount);
        appendComparisonRow(
            stream, "Page faults", snapshot_.pageFaultCount,
            baseline.pageFaultCount);
        stream << "Fault rate: " << std::fixed << std::setprecision(1)
               << pageFaultRatePercent(baseline) << "% -> "
               << pageFaultRatePercent(snapshot_) << "% ("
               << pageFaultRatePercent(snapshot_) -
                      pageFaultRatePercent(baseline)
               << " pp)\n";
        appendComparisonRow(
            stream, "Page replacements", snapshot_.pageReplacementCount,
            baseline.pageReplacementCount);
        appendComparisonRow(
            stream, "Dirty-page evictions", snapshot_.dirtyEvictionCount,
            baseline.dirtyEvictionCount);
        stream << "Utilization: " << std::fixed << std::setprecision(1)
               << baseline.memory.utilizationPercent << "% -> "
               << snapshot_.memory.utilizationPercent << "% ("
               << snapshot_.memory.utilizationPercent -
                      baseline.memory.utilizationPercent
               << " pp)\n";
        stream << "Execution time: " << milliseconds(baseline.executionTime)
               << " -> " << milliseconds(snapshot_.executionTime) << " ms ("
               << milliseconds(snapshot_.executionTime - baseline.executionTime)
               << " ms)\n";
    }

    return stream.str();
}

std::string SimulationStatisticsVisualizationView::renderCompact() const
{
    if (!hasSnapshot_)
    {
        return "{no statistics}";
    }

    std::ostringstream stream;
    stream << "{" << snapshot_.replacementPolicy
           << " accesses=" << snapshot_.memoryAccessCount << "/"
           << snapshot_.totalAccessCount
           << " faults=" << snapshot_.pageFaultCount
           << " replacements=" << snapshot_.pageReplacementCount
           << " dirty=" << snapshot_.dirtyEvictionCount
           << " memory=" << std::fixed << std::setprecision(1)
           << snapshot_.memory.utilizationPercent << "%}";
    return stream.str();
}

} // namespace emmus::gui