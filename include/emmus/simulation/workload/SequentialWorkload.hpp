#pragma once

#include <cstddef>
#include <vector>

#include "emmus/simulation/workload/IWorkload.hpp"

namespace emmus::simulation::workload {

class SequentialWorkload final : public IWorkload {
public:
    SequentialWorkload(
        memory::identifiers::ProcessId processId,
        std::size_t pageCount,
        memory::access::PageSize pageSize,
        std::size_t accessCount);

    [[nodiscard]] bool hasNext() const noexcept override;

    memory::access::MemoryAccess nextAccess() override;

    void reset() override;

    [[nodiscard]] std::size_t size() const noexcept override;

private:
    std::vector<memory::access::MemoryAccess> accesses_;
    std::size_t nextIndex_{0};
};

} // namespace emmus::simulation::workload