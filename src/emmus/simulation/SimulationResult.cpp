#include "emmus/simulation/SimulationResult.hpp"

namespace emmus::simulation {

SimulationResult::SimulationResult(
    infrastructure::configuration::SimulationConfiguration configuration,
    application::MemoryAccessExecutionResult executionResult,
    std::chrono::nanoseconds executionTime,
    SimulationStatus status,
    std::string diagnostic)
    : configuration_(std::move(configuration)),
      executionResult_(std::move(executionResult)),
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