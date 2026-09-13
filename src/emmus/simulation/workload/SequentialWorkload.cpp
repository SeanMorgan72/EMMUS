#include "emmus/simulation/workload/SequentialWorkload.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace emmus::simulation::workload
{

SequentialWorkload::SequentialWorkload(
    const ProcessId processId,
    const std::size_t pageCount,
    const PageSize pageSize,
    const std::size_t accessCount
)
    : SequentialWorkload(
        processId,
        pageCount,
        pageSize,
        accessCount,
        PageId{0U},
        pageCount
    )
{
}


SequentialWorkload::SequentialWorkload(
    const ProcessId processId,
    const std::size_t pageCount,
    const PageSize pageSize,
    const std::size_t accessCount,
    const PageId startingPage,
    const std::size_t patternLength
)
{
    if (pageCount == 0U)
    {
        throw std::invalid_argument(
            "Sequential workload requires at least one page."
        );
    }

    if (accessCount == 0U)
    {
        throw std::invalid_argument(
            "Sequential workload requires at least one access."
        );
    }

    if (patternLength == 0U)
    {
        throw std::invalid_argument(
            "Sequential workload requires a non-zero pattern length."
        );
    }

    const auto pageCountValue =
        static_cast<std::uint64_t>(pageCount);

    const auto startingPageValue =
        startingPage.value();

    /*
     * startingPage must identify a page inside the process address space.
     */
    if (startingPageValue >= pageCountValue)
    {
        throw std::invalid_argument(
            "Sequential workload starting page is outside the process "
            "address space."
        );
    }

    /*
     * The complete sequential pattern must remain inside the configured
     * process address space.
     *
     * Using subtraction rather than startingPage + patternLength avoids
     * unsigned-integer overflow during validation.
     */
    const auto remainingPages =
        pageCountValue - startingPageValue;

    if (static_cast<std::uint64_t>(patternLength) > remainingPages)
    {
        throw std::invalid_argument(
            "Sequential workload pattern extends beyond the process "
            "address space."
        );
    }

    const auto pageSizeValue =
        pageSize.value();

    /*
     * The workload generates page-aligned references at offset zero.
     * Validate the largest generated address before constructing the
     * access sequence.
     */
    const auto lastPatternPage =
        startingPageValue +
        static_cast<std::uint64_t>(patternLength - 1U);

    if (lastPatternPage >
        std::numeric_limits<std::uint64_t>::max() / pageSizeValue)
    {
        throw std::invalid_argument(
            "Sequential workload virtual address would overflow."
        );
    }

    accesses_.reserve(accessCount);

    for (std::size_t index = 0U; index < accessCount; ++index)
    {
        const auto patternOffset =
            index % patternLength;

        const auto pageIndex =
            startingPageValue +
            static_cast<std::uint64_t>(patternOffset);

        const auto virtualAddress =
            memory::access::VirtualAddress{
                pageIndex * pageSizeValue
            };

        accesses_.emplace_back(
            processId,
            virtualAddress,
            memory::access::MemoryAccessOperation::Read,
            memory::access::AccessSequenceNumber{
                static_cast<std::uint64_t>(index)
            }
        );
    }
}


bool SequentialWorkload::hasNext() const noexcept
{
    return nextIndex_ < accesses_.size();
}


memory::access::MemoryAccess SequentialWorkload::nextAccess()
{
    if (!hasNext())
    {
        throw std::out_of_range(
            "Sequential workload has no remaining accesses."
        );
    }

    return accesses_[nextIndex_++];
}


void SequentialWorkload::reset()
{
    nextIndex_ = 0U;
}


std::size_t SequentialWorkload::size() const noexcept
{
    return accesses_.size();
}

} // namespace emmus::simulation::workload