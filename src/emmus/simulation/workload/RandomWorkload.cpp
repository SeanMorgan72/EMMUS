#include "emmus/simulation/workload/RandomWorkload.hpp"

#include <stdexcept>

namespace emmus::simulation::workload {

RandomWorkload::RandomWorkload(
    memory::identifiers::ProcessId processId,
    std::size_t pageCount,
    memory::access::PageSize pageSize,
    std::size_t accessCount,
    std::uint64_t seed)
    : processId_(processId),
      pageCount_(pageCount),
      pageSize_(pageSize),
      accessCount_(accessCount),
      seed_(seed) {

    if (pageCount_ == 0U) {
        throw std::invalid_argument(
            "Random workload requires at least one page.");
    }

    generate();
}

void RandomWorkload::generate() {
    accesses_.clear();
    accesses_.reserve(accessCount_);

    std::mt19937_64 generator(seed_);
    std::uniform_int_distribution<std::size_t> pageDistribution(
        0U,
        pageCount_ - 1U);

    for (std::size_t i = 0; i < accessCount_; ++i) {
        const auto pageIndex = pageDistribution(generator);

        const auto virtualAddress =
            memory::access::VirtualAddress{
                static_cast<std::uint64_t>(pageIndex) *
                static_cast<std::uint64_t>(pageSize_.value())
            };

        accesses_.emplace_back(
            processId_,
            virtualAddress,
            memory::access::MemoryAccessOperation::Read,
            memory::access::AccessSequenceNumber{
                static_cast<std::uint64_t>(i)
            });
    }
}

bool RandomWorkload::hasNext() const noexcept {
    return nextIndex_ < accesses_.size();
}

memory::access::MemoryAccess RandomWorkload::nextAccess() {
    if (!hasNext()) {
        throw std::out_of_range(
            "Random workload has no remaining accesses.");
    }

    return accesses_[nextIndex_++];
}

void RandomWorkload::reset() {
    nextIndex_ = 0;
}

std::size_t RandomWorkload::size() const noexcept {
    return accesses_.size();
}

} // namespace emmus::simulation::workload