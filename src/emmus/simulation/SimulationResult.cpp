#include "emmus/simulation/SimulationResult.hpp"

#include <utility>

namespace emmus::simulation {

SimulationResult::SimulationResult(
    infrastructure::configuration::SimulationConfiguration configuration,
    application::MemoryAccessExecutionResult executionResult,
    emmus::statistics::PageFaultStatistics pageFaultStatistics,
    emmus::statistics::PageReplacementStatistics pageReplacementStatistics,
    emmus::simulation::activity::SimulationActivityLog activityLog,
    std::chrono::nanoseconds executionTime,
    SimulationStatus status,
    std::string diagnostic)
    : configuration_(std::move(configuration)),
      executionResult_(std::move(executionResult)),
      pageFaultStatistics_(std::move(pageFaultStatistics)),
      pageReplacementStatistics_(std::move(pageReplacementStatistics)),
      activityLog_(std::move(activityLog)),
      executionTime_(executionTime),
      status_(status),
      diagnostic_(std::move(diagnostic)) {}

const infrastructure::configuration::SimulationConfiguration&
SimulationResult::configuration() const noexcept {
    return configuration_;
}

const application::MemoryAccessExecutionResult&
SimulationResult::executionResult() const noexcept {
    return executionResult_;
}

const emmus::statistics::PageFaultStatistics&
SimulationResult::pageFaultStatistics() const noexcept {
    return pageFaultStatistics_;
}

const emmus::statistics::PageReplacementStatistics&
SimulationResult::pageReplacementStatistics() const noexcept {
    return pageReplacementStatistics_;
}

const emmus::simulation::activity::SimulationActivityLog&
SimulationResult::activityLog() const noexcept {
    return activityLog_;
}

std::chrono::nanoseconds SimulationResult::executionTime() const noexcept {
    return executionTime_;
}

SimulationStatus SimulationResult::status() const noexcept {
    return status_;
}

const std::string& SimulationResult::diagnostic() const noexcept {
    return diagnostic_;
}

bool SimulationResult::succeeded() const noexcept {
    return status_ == SimulationStatus::Completed;
}

} // namespace emmus::simulation