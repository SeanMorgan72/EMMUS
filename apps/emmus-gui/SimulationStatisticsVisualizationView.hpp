#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include "emmus/memory/physical/PhysicalMemoryUtilization.hpp"

namespace emmus::simulation
{
struct SimulationProgress;
class SimulationResult;
}

namespace emmus::gui
{

class SimulationStatisticsVisualizationView final
{
public:
    using Utilization = emmus::memory::physical::PhysicalMemoryUtilization;
    using Duration = std::chrono::nanoseconds;
    using Counter = std::uint64_t;

    enum class Status
    {
        Running,
        Completed,
        Failed
    };

    struct Snapshot
    {
        std::string replacementPolicy;
        Status status{Status::Running};
        Counter memoryAccessCount{0U};
        Counter totalAccessCount{0U};
        Counter successfulAccessCount{0U};
        Counter failedAccessCount{0U};
        Counter pageFaultCount{0U};
        Counter pageReplacementCount{0U};
        Counter dirtyEvictionCount{0U};
        Duration executionTime{Duration::zero()};
        Duration replacementExecutionTime{Duration::zero()};
        Utilization memory{};
    };

    void update(const emmus::simulation::SimulationProgress& progress);

    void update(
        const emmus::simulation::SimulationResult& result,
        const Utilization& memory);

    void setComparison(Snapshot baseline);

    void clearComparison() noexcept;

    void reset() noexcept;

    [[nodiscard]] bool hasSnapshot() const noexcept;

    [[nodiscard]] const Snapshot& snapshot() const noexcept;

    [[nodiscard]] const std::optional<Snapshot>& comparison() const noexcept;

    [[nodiscard]] std::string render() const;

    [[nodiscard]] std::string renderCompact() const;

private:
    Snapshot snapshot_{};
    std::optional<Snapshot> comparison_{};
    bool hasSnapshot_{false};
};

} // namespace emmus::gui