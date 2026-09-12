#include "emmus/simulation/workload/SequentialWorkload.hpp"

#include <stdexcept>

namespace emmus::simulation::workload {

SequentialWorkload::SequentialWorkload(
    memory::identifiers::ProcessId processId,
    std::size_t pageCount,
    memory::access::PageSize pageSize,
    std::size_t accessCount) {

    if (pageCount == 0U) {
        throw std::invalid_argument(
            "Sequential workload requires at least one page.");
    }

    accesses_.reserve(accessCount);

    for (std::size_t i = 0; i < accessCount; ++i) {
        const auto pageIndex = i % pageCount;

        const auto virtualAddress =
            memory::access::VirtualAddress{
                static_cast<std::uint64_t>(pageIndex) *
                static_cast<std::uint64_t>(pageSize.value())
            };

        accesses_.emplace_back(
            processId,
            virtualAddress,
            memory::access::MemoryAccessOperation::Read,
            memory::access::AccessSequenceNumber{
                static_cast<std::uint64_t>(i)
            });
    }
}

bool SequentialWorkload::hasNext() const noexcept {
    return nextIndex_ < accesses_.size();
}

memory::access::MemoryAccess SequentialWorkload::nextAccess() {
    if (!hasNext()) {
        throw std::out_of_range(
            "Sequential workload has no remaining accesses.");
    }

    return accesses_[nextIndex_++];
}

void SequentialWorkload::reset() {
    nextIndex_ = 0;
}

std::size_t SequentialWorkload::size() const noexcept {
    return accesses_.size();
}

} // namespace emmus::simulation::workload