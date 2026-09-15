#pragma once

#include <cstddef>
#include <cstdint>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"

namespace emmus::infrastructure::configuration {

enum class WorkloadType : std::uint8_t {
    Sequential,
    Random,
    Locality
};

class SimulationConfiguration final {
public:
    SimulationConfiguration(
        memory::access::PageSize pageSize,
        memory::access::FrameCount frameCount,
        std::size_t processCount,
        std::size_t pageCountPerProcess,
        algorithms::replacement::PageReplacementPolicyType replacementPolicy,
        WorkloadType workloadType,
        std::size_t memoryAccessCount,
        std::uint64_t randomSeed);

    SimulationConfiguration(
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
        std::size_t workingSetSize);

    [[nodiscard]] memory::access::PageSize pageSize() const noexcept;

    [[nodiscard]] memory::access::FrameCount frameCount() const noexcept;

    [[nodiscard]] std::size_t processCount() const noexcept;

    [[nodiscard]] std::size_t pageCountPerProcess() const noexcept;

    [[nodiscard]]
    algorithms::replacement::PageReplacementPolicyType
    replacementPolicy() const noexcept;

    [[nodiscard]] WorkloadType workloadType() const noexcept;

    [[nodiscard]] std::size_t memoryAccessCount() const noexcept;

    [[nodiscard]] std::uint64_t randomSeed() const noexcept;

    [[nodiscard]] double temporalLocalityStrength() const noexcept;

    [[nodiscard]] double spatialLocalityStrength() const noexcept;

    [[nodiscard]] std::size_t workingSetSize() const noexcept;

private:
    memory::access::PageSize pageSize_;
    memory::access::FrameCount frameCount_;
    std::size_t processCount_;
    std::size_t pageCountPerProcess_;

    algorithms::replacement::PageReplacementPolicyType
        replacementPolicy_;

    WorkloadType workloadType_;
    std::size_t memoryAccessCount_;
    std::uint64_t randomSeed_;

    double temporalLocalityStrength_;
    double spatialLocalityStrength_;
    std::size_t workingSetSize_;
};

} // namespace emmus::infrastructure::configuration