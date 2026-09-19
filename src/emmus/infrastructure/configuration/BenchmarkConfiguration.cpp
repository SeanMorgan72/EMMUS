#include "emmus/infrastructure/configuration/BenchmarkConfiguration.hpp"

#include <utility>

#include "emmus/infrastructure/configuration/SimulationConfigurationValidator.hpp"

namespace emmus::infrastructure::configuration {

BenchmarkConfiguration::BenchmarkConfiguration(
    memory::access::PageSize pageSize,
    memory::access::FrameCount frameCount,
    std::size_t processCount,
    std::size_t pageCountPerProcess,
    std::size_t memoryAccessCount,
    WorkloadType workloadType,
    std::uint64_t randomSeed,
    algorithms::replacement::PageReplacementPolicyType replacementPolicy)
    : pageSize_(pageSize),
      frameCount_(frameCount),
      processCount_(processCount),
      pageCountPerProcess_(pageCountPerProcess),
      memoryAccessCount_(memoryAccessCount),
      workloadType_(workloadType),
      randomSeed_(randomSeed),
      replacementPolicy_(replacementPolicy) {
}

memory::access::PageSize
BenchmarkConfiguration::pageSize() const noexcept {
    return pageSize_;
}

memory::access::FrameCount
BenchmarkConfiguration::frameCount() const noexcept {
    return frameCount_;
}

std::size_t BenchmarkConfiguration::processCount() const noexcept {
    return processCount_;
}

std::size_t BenchmarkConfiguration::pageCountPerProcess() const noexcept {
    return pageCountPerProcess_;
}

std::size_t BenchmarkConfiguration::memoryAccessCount() const noexcept {
    return memoryAccessCount_;
}

WorkloadType BenchmarkConfiguration::workloadType() const noexcept {
    return workloadType_;
}

std::uint64_t BenchmarkConfiguration::randomSeed() const noexcept {
    return randomSeed_;
}

algorithms::replacement::PageReplacementPolicyType
BenchmarkConfiguration::replacementPolicy() const noexcept {
    return replacementPolicy_;
}

std::vector<std::string> BenchmarkConfiguration::validate() const {
    SimulationConfiguration configuration(
        pageSize_,
        frameCount_,
        processCount_,
        pageCountPerProcess_,
        replacementPolicy_,
        workloadType_,
        memoryAccessCount_,
        randomSeed_);

    return SimulationConfigurationValidator::validate(configuration);
}

bool BenchmarkConfiguration::isValid() const noexcept {
    return validate().empty();
}

SimulationConfiguration BenchmarkConfiguration::toSimulationConfiguration() const {
    const auto errors = validate();
    if (!errors.empty()) {
        throw std::invalid_argument(errors.front());
    }

    SimulationConfiguration configuration(
        pageSize_,
        frameCount_,
        processCount_,
        pageCountPerProcess_,
        replacementPolicy_,
        workloadType_,
        memoryAccessCount_,
        randomSeed_);

    return configuration;
}

BenchmarkConfiguration BenchmarkConfiguration::withReplacementPolicy(
    algorithms::replacement::PageReplacementPolicyType replacementPolicy)
    const {
    return BenchmarkConfiguration(
        pageSize_,
        frameCount_,
        processCount_,
        pageCountPerProcess_,
        memoryAccessCount_,
        workloadType_,
        randomSeed_,
        replacementPolicy);
}

} // namespace emmus::infrastructure::configuration
