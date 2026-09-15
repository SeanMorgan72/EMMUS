#include "emmus/infrastructure/configuration/SimulationConfigurationValidator.hpp"

#include <cmath>
#include <limits>
#include <string>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"

namespace emmus::infrastructure::configuration {

std::vector<std::string>
SimulationConfigurationValidator::validate(
    const SimulationConfiguration& configuration)
{
    std::vector<std::string> errors;

    // General configuration validation.
    if (configuration.frameCount().value() == 0) {
        errors.emplace_back(
            "Physical frame count must be greater than zero.");
    }

    if (configuration.processCount() == 0) {
        errors.emplace_back(
            "Process count must be greater than zero.");
    }

    if (configuration.pageCountPerProcess() == 0) {
        errors.emplace_back(
            "Page count per process must be greater than zero.");
    }

    if (configuration.memoryAccessCount() == 0) {
        errors.emplace_back(
            "Memory access count must be greater than zero.");
    }

    // Replacement-policy validation.
    switch (configuration.replacementPolicy()) {
    case algorithms::replacement::PageReplacementPolicyType::FIFO:
    case algorithms::replacement::PageReplacementPolicyType::LRU:
    case algorithms::replacement::PageReplacementPolicyType::CLOCK:
    case algorithms::replacement::PageReplacementPolicyType::OPTIMAL:
        break;

    default:
        errors.emplace_back(
            "Unsupported page replacement policy.");
        break;
    }

    // Locality-workload-specific validation.
    if (configuration.workloadType() == WorkloadType::Locality) {
        const double temporalStrength =
            configuration.temporalLocalityStrength();

        const double spatialStrength =
            configuration.spatialLocalityStrength();

        if (!std::isfinite(temporalStrength) ||
            temporalStrength < 0.0 ||
            temporalStrength > 1.0) {

            errors.emplace_back(
                "Temporal locality strength must be between zero and one.");
        }

        if (!std::isfinite(spatialStrength) ||
            spatialStrength < 0.0 ||
            spatialStrength > 1.0) {

            errors.emplace_back(
                "Spatial locality strength must be between zero and one.");
        }

        if (configuration.workingSetSize() == 0) {
            errors.emplace_back(
                "Locality working-set size must be greater than zero.");
        } else if (
            configuration.workingSetSize() >
            configuration.pageCountPerProcess()) {

            errors.emplace_back(
                "Locality working-set size must not exceed "
                "page count per process.");
        }
    }

    // Mixed-workload-specific validation.
    if (configuration.workloadType() == WorkloadType::Mixed) {
        const auto& segments =
            configuration.mixedWorkloadSegments();

        if (segments.empty()) {
            errors.emplace_back(
                "Mixed workload must contain at least one segment.");
        } else {
            std::size_t totalAccesses = 0;

            for (std::size_t index = 0;
                 index < segments.size();
                 ++index) {

                const auto& segment =
                    segments[index];

                if (segment.accessCount == 0) {
                    errors.emplace_back(
                        "Mixed workload segment " +
                        std::to_string(index) +
                        " must contain at least one access.");
                }

                if (segment.workloadType ==
                    WorkloadType::Mixed) {

                    errors.emplace_back(
                        "Mixed workload segments cannot contain "
                        "another mixed workload.");
                }

                if (segment.accessCount >
                    std::numeric_limits<std::size_t>::max() -
                        totalAccesses) {

                    errors.emplace_back(
                        "Mixed workload access count exceeds "
                        "size_t range.");

                    break;
                }

                totalAccesses +=
                    segment.accessCount;
            }

            if (totalAccesses == 0) {
                errors.emplace_back(
                    "Mixed workload must contain at least one access.");
            }
        }
    }

    return errors;
}

bool SimulationConfigurationValidator::isValid(
    const SimulationConfiguration& configuration)
{
    return validate(configuration).empty();
}

} // namespace emmus::infrastructure::configuration