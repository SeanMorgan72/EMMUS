#include "emmus/infrastructure/configuration/SimulationConfigurationValidator.hpp"

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"

namespace emmus::infrastructure::configuration {

std::vector<std::string> SimulationConfigurationValidator::validate(
    const SimulationConfiguration& configuration) {

    std::vector<std::string> errors;

    if (configuration.frameCount().value() == 0U) {
        errors.emplace_back(
            "Physical memory frame count must be greater than zero.");
    }

    if (configuration.processCount() == 0U) {
        errors.emplace_back(
            "Process count must be greater than zero.");
    }

    if (configuration.pageCountPerProcess() == 0U) {
        errors.emplace_back(
            "Page count per process must be greater than zero.");
    }

    if (configuration.memoryAccessCount() == 0U) {
        errors.emplace_back(
            "Memory access count must be greater than zero.");
    }

    const auto policy = configuration.replacementPolicy();

    const bool validPolicy =
        policy ==
            emmus::algorithms::replacement::PageReplacementPolicyType::FIFO ||
        policy ==
            emmus::algorithms::replacement::PageReplacementPolicyType::LRU ||
        policy ==
            emmus::algorithms::replacement::PageReplacementPolicyType::CLOCK ||
        policy ==
            emmus::algorithms::replacement::PageReplacementPolicyType::OPTIMAL;

    if (!validPolicy) {
        errors.emplace_back(
            "Unsupported page replacement policy.");
    }

    return errors;
}

bool SimulationConfigurationValidator::isValid(
    const SimulationConfiguration& configuration) {

    return validate(configuration).empty();
}

} // namespace emmus::infrastructure::configuration