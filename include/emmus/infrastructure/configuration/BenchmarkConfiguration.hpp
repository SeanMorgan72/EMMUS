#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"

namespace emmus::infrastructure::configuration {

class BenchmarkConfiguration final {
public:
    BenchmarkConfiguration(
        memory::access::PageSize pageSize,
        memory::access::FrameCount frameCount,
        std::size_t processCount,
        std::size_t pageCountPerProcess,
        std::size_t memoryAccessCount,
        WorkloadType workloadType,
        std::uint64_t randomSeed,
        algorithms::replacement::PageReplacementPolicyType replacementPolicy);

    [[nodiscard]] memory::access::PageSize pageSize() const noexcept;

    [[nodiscard]] memory::access::FrameCount frameCount() const noexcept;

    [[nodiscard]] std::size_t processCount() const noexcept;

    [[nodiscard]] std::size_t pageCountPerProcess() const noexcept;

    [[nodiscard]] std::size_t memoryAccessCount() const noexcept;

    [[nodiscard]] WorkloadType workloadType() const noexcept;

    [[nodiscard]] std::uint64_t randomSeed() const noexcept;

    [[nodiscard]]
    algorithms::replacement::PageReplacementPolicyType
    replacementPolicy() const noexcept;

    [[nodiscard]] std::vector<std::string> validate() const;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] SimulationConfiguration toSimulationConfiguration() const;

    [[nodiscard]] BenchmarkConfiguration withReplacementPolicy(
        algorithms::replacement::PageReplacementPolicyType replacementPolicy)
        const;

private:
    memory::access::PageSize pageSize_;
    memory::access::FrameCount frameCount_;
    std::size_t processCount_;
    std::size_t pageCountPerProcess_;
    std::size_t memoryAccessCount_;
    WorkloadType workloadType_;
    std::uint64_t randomSeed_;
    algorithms::replacement::PageReplacementPolicyType replacementPolicy_;
};

} // namespace emmus::infrastructure::configuration
