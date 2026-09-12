#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "emmus/simulation/workload/IWorkload.hpp"

namespace emmus::simulation::workload {

class MultiProcessWorkload final : public IWorkload {
public:
    explicit MultiProcessWorkload(
        std::vector<std::unique_ptr<IWorkload>> workloads);

    [[nodiscard]] bool hasNext() const noexcept override;

    memory::access::MemoryAccess nextAccess() override;

    void reset() override;

    [[nodiscard]] std::size_t size() const noexcept override;

private:
    std::vector<std::unique_ptr<IWorkload>> workloads_;
    mutable std::size_t currentWorkload_{0};
    std::size_t totalSize_{0};
};

} // namespace emmus::simulation::workload