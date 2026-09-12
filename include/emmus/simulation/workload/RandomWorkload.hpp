#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "emmus/simulation/workload/IWorkload.hpp"

namespace emmus::simulation::workload {

class RandomWorkload final : public IWorkload {
public:
    RandomWorkload(
        memory::identifiers::ProcessId processId,
        std::size_t pageCount,
        memory::access::PageSize pageSize,
        std::size_t accessCount,
        std::uint64_t seed);

    [[nodiscard]] bool hasNext() const noexcept override;

    memory::access::MemoryAccess nextAccess() override;

    void reset() override;

    [[nodiscard]] std::size_t size() const noexcept override;

private:
    void generate();

    memory::identifiers::ProcessId processId_;
    std::size_t pageCount_;
    memory::access::PageSize pageSize_;
    std::size_t accessCount_;
    std::uint64_t seed_;

    std::vector<memory::access::MemoryAccess> accesses_;
    std::size_t nextIndex_{0};
};

} // namespace emmus::simulation::workload