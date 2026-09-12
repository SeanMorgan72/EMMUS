#pragma once

#include <chrono>
#include <cstddef>
#include <string>

#include "emmus/application/MemoryAccessExecutionResult.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"

namespace emmus::simulation {

enum class SimulationStatus {
    Completed,
    Failed
};

class SimulationResult final {
public:
    SimulationResult(
        infrastructure::configuration::SimulationConfiguration configuration,
        application::MemoryAccessExecutionResult executionResult,
        std::chrono::nanoseconds executionTime,
        SimulationStatus status,
        std::string diagnostic = {});

    [[nodiscard]] const infrastructure::configuration::
        SimulationConfiguration& configuration() const noexcept;

    [[nodiscard]] const application::MemoryAccessExecutionResult&
    executionResult() const noexcept;

    [[nodiscard]] std::chrono::nanoseconds executionTime() const noexcept;

    [[nodiscard]] SimulationStatus status() const noexcept;

    [[nodiscard]] const std::string& diagnostic() const noexcept;

    [[nodiscard]] bool succeeded() const noexcept;

private:
    infrastructure::configuration::SimulationConfiguration configuration_;
    application::MemoryAccessExecutionResult executionResult_;
    std::chrono::nanoseconds executionTime_;
    SimulationStatus status_;
    std::string diagnostic_;
};

} // namespace emmus::simulation