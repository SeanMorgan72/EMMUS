#include "emmus/simulation/workload/LocalityWorkload.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace emmus::simulation::workload {

namespace {

constexpr std::size_t MaximumRecentPages = 16U;

} // namespace

LocalityWorkload::LocalityWorkload(
    memory::identifiers::ProcessId processId,
    std::size_t pageCount,
    memory::access::PageSize pageSize,
    std::size_t accessCount,
    double temporalLocalityStrength,
    double spatialLocalityStrength,
    std::size_t workingSetSize,
    std::uint64_t seed)
    : processId_(processId),
      pageCount_(pageCount),
      pageSize_(pageSize),
      accessCount_(accessCount),
      temporalLocalityStrength_(temporalLocalityStrength),
      spatialLocalityStrength_(spatialLocalityStrength),
      workingSetSize_(workingSetSize),
      seed_(seed) {

    if (pageCount_ == 0U) {
        throw std::invalid_argument(
            "Locality workload page count must be greater than zero.");
    }

    if (accessCount_ == 0U) {
        throw std::invalid_argument(
            "Locality workload access count must be greater than zero.");
    }

    if (temporalLocalityStrength_ < 0.0 ||
        temporalLocalityStrength_ > 1.0) {
        throw std::invalid_argument(
            "Temporal locality strength must be between zero and one.");
    }

    if (spatialLocalityStrength_ < 0.0 ||
        spatialLocalityStrength_ > 1.0) {
        throw std::invalid_argument(
            "Spatial locality strength must be between zero and one.");
    }

    if (workingSetSize_ == 0U) {
        throw std::invalid_argument(
            "Locality workload working-set size must be greater than zero.");
    }

    if (workingSetSize_ > pageCount_) {
        throw std::invalid_argument(
            "Locality workload working-set size must not exceed page count.");
    }

    const auto maximumPageIndex =
        static_cast<std::uint64_t>(pageCount_ - 1U);

    if (maximumPageIndex >
        std::numeric_limits<std::uint64_t>::max() /
            pageSize_.value()) {
        throw std::invalid_argument(
            "Locality workload address range exceeds the supported "
            "virtual address space.");
    }

    generate();
}

bool LocalityWorkload::hasNext() const noexcept {
    return nextIndex_ < accesses_.size();
}

memory::access::MemoryAccess LocalityWorkload::nextAccess() {
    if (!hasNext()) {
        throw std::out_of_range(
            "No more memory accesses are available.");
    }

    return accesses_[nextIndex_++];
}

void LocalityWorkload::reset() {
    nextIndex_ = 0U;
}

std::size_t LocalityWorkload::size() const noexcept {
    return accesses_.size();
}

void LocalityWorkload::generate() {
    accesses_.clear();
    accesses_.reserve(accessCount_);

    std::mt19937_64 generator(seed_);

    const std::size_t workingSetStart =
        selectWorkingSetStart(generator);

    std::vector<PageIndex> recentPages;
    recentPages.reserve(MaximumRecentPages);

    for (std::size_t accessIndex = 0U;
         accessIndex < accessCount_;
         ++accessIndex) {

        const PageIndex relativePage =
            generatePageIndex(generator, recentPages);

        const PageIndex pageIndex =
            workingSetStart + relativePage;

        const auto virtualAddress =
            memory::access::VirtualAddress{
                static_cast<std::uint64_t>(pageIndex) *
                pageSize_.value()};

        accesses_.emplace_back(
            processId_,
            virtualAddress,
            memory::access::MemoryAccessOperation::Read,
            memory::access::AccessSequenceNumber{
                static_cast<std::uint64_t>(accessIndex)});

        if (recentPages.size() == MaximumRecentPages) {
            recentPages.erase(recentPages.begin());
        }

        recentPages.push_back(pageIndex - workingSetStart);
    }
}

LocalityWorkload::PageIndex
LocalityWorkload::generatePageIndex(
    std::mt19937_64& generator,
    std::vector<PageIndex>& recentPages) const {

    std::uniform_real_distribution<double> probabilityDistribution(
        0.0,
        1.0);

    const double probability =
        probabilityDistribution(generator);

    /*
     * The temporal probability is evaluated first.
     *
     * The spatial probability applies to the remaining probability
     * mass. This allows both dimensions to be configured independently
     * while ensuring their combined probability never exceeds one.
     */
    if (!recentPages.empty() &&
        probability < temporalLocalityStrength_) {

        return generateTemporalPageIndex(
            generator,
            recentPages);
    }

    const double remainingProbability =
        probability - temporalLocalityStrength_;

    if (remainingProbability >= 0.0 &&
        remainingProbability <
            (1.0 - temporalLocalityStrength_) *
                spatialLocalityStrength_) {

        const PageIndex previousPage =
            recentPages.empty()
                ? 0U
                : recentPages.back();

        return generateSpatialPageIndex(
            generator,
            previousPage);
    }

    return generateUniformPageIndex(generator);
}

LocalityWorkload::PageIndex
LocalityWorkload::generateUniformPageIndex(
    std::mt19937_64& generator) const {

    std::uniform_int_distribution<PageIndex> distribution(
        0U,
        workingSetSize_ - 1U);

    return distribution(generator);
}

LocalityWorkload::PageIndex
LocalityWorkload::generateTemporalPageIndex(
    std::mt19937_64& generator,
    const std::vector<PageIndex>& recentPages) const {

    std::uniform_int_distribution<std::size_t> distribution(
        0U,
        recentPages.size() - 1U);

    return recentPages[distribution(generator)];
}

LocalityWorkload::PageIndex
LocalityWorkload::generateSpatialPageIndex(
    std::mt19937_64& generator,
    PageIndex previousPage) const {

    if (workingSetSize_ <= 1U) {
        return previousPage;
    }

    constexpr std::size_t SpatialRadius = 2U;

    const PageIndex lowerBound =
        previousPage > SpatialRadius
            ? previousPage - SpatialRadius
            : 0U;

    const PageIndex upperBound =
        std::min(
            workingSetSize_ - 1U,
            previousPage + SpatialRadius);

    std::uniform_int_distribution<PageIndex> distribution(
        lowerBound,
        upperBound);

    return distribution(generator);
}

LocalityWorkload::PageIndex
LocalityWorkload::selectWorkingSetStart(
    std::mt19937_64& generator) const {

    const std::size_t maximumStart =
        pageCount_ - workingSetSize_;

    if (maximumStart == 0U) {
        return 0U;
    }

    std::uniform_int_distribution<PageIndex> distribution(
        0U,
        maximumStart);

    return distribution(generator);
}

} // namespace emmus::simulation::workload