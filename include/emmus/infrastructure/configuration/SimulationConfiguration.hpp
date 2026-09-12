#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"

namespace emmus::infrastructure::configuration {

enum class WorkloadType : std::uint8_t {
    Sequential,
    Random
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

    [[nodiscard]] memory::access::PageSize pageSize() const noexcept;
    [[nodiscard]] memory::access::FrameCount frameCount() const noexcept;
    [[nodiscard]] std::size_t processCount() const noexcept;
    [[nodiscard]] std::size_t pageCountPerProcess() const noexcept;

    [[nodiscard]] algorithms::replacement::PageReplacementPolicyType
    replacementPolicy() const noexcept;

    [[nodiscard]] WorkloadType workloadType() const noexcept;
    [[nodiscard]] std::size_t memoryAccessCount() const noexcept;
    [[nodiscard]] std::uint64_t randomSeed() const noexcept;

private:
    memory::access::PageSize pageSize_;
    memory::access::FrameCount frameCount_;
    std::size_t processCount_;
    std::size_t pageCountPerProcess_;
    algorithms::replacement::PageReplacementPolicyType replacementPolicy_;
    WorkloadType workloadType_;
    std::size_t memoryAccessCount_;
    std::uint64_t randomSeed_;
};

} // namespace emmus::infrastructure::configuration