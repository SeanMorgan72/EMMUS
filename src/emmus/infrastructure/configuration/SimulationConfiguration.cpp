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
    : pageSize_(pageSize),
      frameCount_(frameCount),
      processCount_(processCount),
      pageCountPerProcess_(pageCountPerProcess),
      replacementPolicy_(replacementPolicy),
      workloadType_(workloadType),
      memoryAccessCount_(memoryAccessCount),
      randomSeed_(randomSeed) {}

memory::access::PageSize SimulationConfiguration::pageSize() const noexcept {
    return pageSize_;
}

memory::access::FrameCount SimulationConfiguration::frameCount() const noexcept {
    return frameCount_;
}

std::size_t SimulationConfiguration::processCount() const noexcept {
    return processCount_;
}

std::size_t SimulationConfiguration::pageCountPerProcess() const noexcept {
    return pageCountPerProcess_;
}

algorithms::replacement::PageReplacementPolicyType
SimulationConfiguration::replacementPolicy() const noexcept {
    return replacementPolicy_;
}

WorkloadType SimulationConfiguration::workloadType() const noexcept {
    return workloadType_;
}

std::size_t SimulationConfiguration::memoryAccessCount() const noexcept {
    return memoryAccessCount_;
}

std::uint64_t SimulationConfiguration::randomSeed() const noexcept {
    return randomSeed_;
}

} // namespace emmus::infrastructure::configuration