#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "emmus/simulation/workload/IWorkload.hpp"

namespace emmus::simulation::workload {

class MixedWorkload final : public IWorkload {
public:
    enum class WorkloadType : std::uint8_t {
        Sequential,
        Random,
        Locality
    };

    struct SegmentConfiguration {
        WorkloadType type;
        std::size_t accessCount;
    };

    MixedWorkload(
        std::vector<std::unique_ptr<IWorkload>> segments);

    [[nodiscard]] bool hasNext() const noexcept override;

    memory::access::MemoryAccess nextAccess() override;

    void reset() override;

    [[nodiscard]] std::size_t size() const noexcept override;

private:
    std::vector<std::unique_ptr<IWorkload>> segments_;
    std::size_t currentSegment_{0};
    std::size_t totalSize_{0};
};

} // namespace emmus::simulation::workload