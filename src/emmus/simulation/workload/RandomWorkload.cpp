#include "emmus/simulation/workload/RandomWorkload.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

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

    if (pageCount_ == 0) {
        throw std::invalid_argument(
            "RandomWorkload page count must be greater than zero.");
    }

    if (accessCount_ == 0) {
        throw std::invalid_argument(
            "RandomWorkload access count must be greater than zero.");
    }

    const auto maximumPageIndex =
        static_cast<std::uint64_t>(pageCount_ - 1);

    const auto pageSizeValue =
        pageSize_.value();

    if (maximumPageIndex >
        std::numeric_limits<std::uint64_t>::max() / pageSizeValue) {
        throw std::invalid_argument(
            "RandomWorkload address space exceeds the virtual address range.");
    }

    generate();
}

bool RandomWorkload::hasNext() const noexcept {
    return nextIndex_ < accesses_.size();
}

memory::access::MemoryAccess RandomWorkload::nextAccess() {
    if (!hasNext()) {
        throw std::out_of_range(
            "RandomWorkload has no remaining memory accesses.");
    }

    return accesses_[nextIndex_++];
}

void RandomWorkload::reset() {
    nextIndex_ = 0;
}

std::size_t RandomWorkload::size() const noexcept {
    return accesses_.size();
}

void RandomWorkload::generate() {
    accesses_.clear();
    accesses_.reserve(accessCount_);

    std::mt19937_64 generator(seed_);

    const auto distributionUpperBound =
        static_cast<std::uint64_t>(pageCount_ - 1);

    std::uniform_int_distribution<std::uint64_t> pageDistribution(
        0,
        distributionUpperBound);

    for (std::size_t accessIndex = 0;
         accessIndex < accessCount_;
         ++accessIndex) {

        const auto pageIndex = pageDistribution(generator);

        const auto virtualAddressValue =
            pageIndex * pageSize_.value();

        const auto virtualAddress =
            memory::access::VirtualAddress{virtualAddressValue};

        accesses_.emplace_back(
            processId_,
            virtualAddress,
            memory::access::MemoryAccessOperation::Read,
            memory::access::AccessSequenceNumber{
                static_cast<std::uint64_t>(accessIndex)});
    }
}

} // namespace emmus::simulation::workload