#include "emmus/simulation/Process.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace emmus::simulation
{

VirtualAddressSpace::VirtualAddressSpace(
    PageCount pageCount,
    PageSize pageSize
)
    : pageCount_(pageCount)
    , pageSize_(pageSize)
{
    if (pageCount == 0)
    {
        throw std::invalid_argument(
            "Virtual address space must contain at least one page."
        );
    }

    if (pageSize == 0)
    {
        throw std::invalid_argument(
            "Virtual address space page size must be greater than zero."
        );
    }

    if (
        pageCount >
        std::numeric_limits<std::uint64_t>::max() / pageSize
    )
    {
        throw std::invalid_argument(
            "Virtual address space size exceeds the supported range."
        );
    }
}

VirtualAddressSpace::PageCount
VirtualAddressSpace::pageCount() const noexcept
{
    return pageCount_;
}

VirtualAddressSpace::PageSize
VirtualAddressSpace::pageSize() const noexcept
{
    return pageSize_;
}

std::uint64_t
VirtualAddressSpace::sizeInBytes() const noexcept
{
    return pageCount_ * pageSize_;
}

Process::Process(
    ProcessId processId,
    VirtualAddressSpace virtualAddressSpace
)
    : processId_(processId)
    , virtualAddressSpace_(std::move(virtualAddressSpace))
    , state_(ProcessState::Created)
{
}

Process::ProcessId
Process::processId() const noexcept
{
    return processId_;
}

const VirtualAddressSpace&
Process::virtualAddressSpace() const noexcept
{
    return virtualAddressSpace_;
}

ProcessState
Process::state() const noexcept
{
    return state_;
}

void
Process::terminate() noexcept
{
    state_ = ProcessState::Terminated;
}

} // namespace emmus::simulation