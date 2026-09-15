#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "emmus/simulation/workload/IWorkload.hpp"

namespace emmus::simulation::workload {

class LocalityWorkload final : public IWorkload {
public:
    LocalityWorkload(
        memory::identifiers::ProcessId processId,
        std::size_t pageCount,
        memory::access::PageSize pageSize,
        std::size_t accessCount,
        double temporalLocalityStrength,
        double spatialLocalityStrength,
        std::size_t workingSetSize,
        std::uint64_t seed);

    [[nodiscard]] bool hasNext() const noexcept override;

    memory::access::MemoryAccess nextAccess() override;

    void reset() override;

    [[nodiscard]] std::size_t size() const noexcept override;

private:
    using PageIndex = std::size_t;

    void generate();

    [[nodiscard]] PageIndex generatePageIndex(
        std::mt19937_64& generator,
        std::vector<PageIndex>& recentPages) const;

    [[nodiscard]] PageIndex generateUniformPageIndex(
        std::mt19937_64& generator) const;

    [[nodiscard]] PageIndex generateTemporalPageIndex(
        std::mt19937_64& generator,
        const std::vector<PageIndex>& recentPages) const;

    [[nodiscard]] PageIndex generateSpatialPageIndex(
        std::mt19937_64& generator,
        PageIndex previousPage) const;

    [[nodiscard]] PageIndex selectWorkingSetStart(
        std::mt19937_64& generator) const;

    memory::identifiers::ProcessId processId_;
    std::size_t pageCount_;
    memory::access::PageSize pageSize_;
    std::size_t accessCount_;

    double temporalLocalityStrength_;
    double spatialLocalityStrength_;
    std::size_t workingSetSize_;
    std::uint64_t seed_;

    std::vector<memory::access::MemoryAccess> accesses_;
    std::size_t nextIndex_{0};
};

} // namespace emmus::simulation::workload