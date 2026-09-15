#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"

namespace emmus::infrastructure::configuration {

SimulationConfiguration::SimulationConfiguration(
    memory::access::PageSize pageSize,
    memory::access::FrameCount frameCount,
    std::size_t processCount,
    std::size_t pageCountPerProcess,
    algorithms::replacement::PageReplacementPolicyType replacementPolicy,
    WorkloadType workloadType,
    std::size_t memoryAccessCount,
    std::uint64_t randomSeed)
    : SimulationConfiguration(
          pageSize,
          frameCount,
          processCount,
          pageCountPerProcess,
          replacementPolicy,
          workloadType,
          memoryAccessCount,
          randomSeed,
          0.0,
          0.0,
          pageCountPerProcess) {
}

SimulationConfiguration::SimulationConfiguration(
    memory::access::PageSize pageSize,
    memory::access::FrameCount frameCount,
    std::size_t processCount,
    std::size_t pageCountPerProcess,
    algorithms::replacement::PageReplacementPolicyType replacementPolicy,
    WorkloadType workloadType,
    std::size_t memoryAccessCount,
    std::uint64_t randomSeed,
    double temporalLocalityStrength,
    double spatialLocalityStrength,
    std::size_t workingSetSize)
    : pageSize_(pageSize),
      frameCount_(frameCount),
      processCount_(processCount),
      pageCountPerProcess_(pageCountPerProcess),
      replacementPolicy_(replacementPolicy),
      workloadType_(workloadType),
      memoryAccessCount_(memoryAccessCount),
      randomSeed_(randomSeed),
      temporalLocalityStrength_(temporalLocalityStrength),
      spatialLocalityStrength_(spatialLocalityStrength),
      workingSetSize_(workingSetSize) {
}

memory::access::PageSize
SimulationConfiguration::pageSize() const noexcept {
    return pageSize_;
}

memory::access::FrameCount
SimulationConfiguration::frameCount() const noexcept {
    return frameCount_;
}

std::size_t
SimulationConfiguration::processCount() const noexcept {
    return processCount_;
}

std::size_t
SimulationConfiguration::pageCountPerProcess() const noexcept {
    return pageCountPerProcess_;
}

algorithms::replacement::PageReplacementPolicyType
SimulationConfiguration::replacementPolicy() const noexcept {
    return replacementPolicy_;
}

WorkloadType
SimulationConfiguration::workloadType() const noexcept {
    return workloadType_;
}

std::size_t
SimulationConfiguration::memoryAccessCount() const noexcept {
    return memoryAccessCount_;
}

std::uint64_t
SimulationConfiguration::randomSeed() const noexcept {
    return randomSeed_;
}

double
SimulationConfiguration::temporalLocalityStrength() const noexcept {
    return temporalLocalityStrength_;
}

double
SimulationConfiguration::spatialLocalityStrength() const noexcept {
    return spatialLocalityStrength_;
}

std::size_t
SimulationConfiguration::workingSetSize() const noexcept {
    return workingSetSize_;
}

} // namespace emmus::infrastructure::configuration