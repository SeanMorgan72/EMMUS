#include "emmus/infrastructure/configuration/SimulationConfigurationValidator.hpp"

#include <cmath>

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
            emmus::algorithms::replacement::
                PageReplacementPolicyType::FIFO ||
        policy ==
            emmus::algorithms::replacement::
                PageReplacementPolicyType::LRU ||
        policy ==
            emmus::algorithms::replacement::
                PageReplacementPolicyType::CLOCK ||
        policy ==
            emmus::algorithms::replacement::
                PageReplacementPolicyType::OPTIMAL;

    if (!validPolicy) {
        errors.emplace_back(
            "Unsupported page replacement policy.");
    }

    if (configuration.workloadType() == WorkloadType::Locality) {

        const double temporalStrength =
            configuration.temporalLocalityStrength();

        if (!std::isfinite(temporalStrength) ||
            temporalStrength < 0.0 ||
            temporalStrength > 1.0) {

            errors.emplace_back(
                "Temporal locality strength must be between zero and one.");
        }

        const double spatialStrength =
            configuration.spatialLocalityStrength();

        if (!std::isfinite(spatialStrength) ||
            spatialStrength < 0.0 ||
            spatialStrength > 1.0) {

            errors.emplace_back(
                "Spatial locality strength must be between zero and one.");
        }

        if (configuration.workingSetSize() == 0U) {
            errors.emplace_back(
                "Locality working-set size must be greater than zero.");
        }
        else if (
            configuration.workingSetSize() >
            configuration.pageCountPerProcess()) {

            errors.emplace_back(
                "Locality working-set size must not exceed "
                "page count per process.");
        }
    }

    return errors;
}

bool SimulationConfigurationValidator::isValid(
    const SimulationConfiguration& configuration) {

    return validate(configuration).empty();
}

} // namespace emmus::infrastructure::configuration