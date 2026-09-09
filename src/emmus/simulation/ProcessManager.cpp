#include "emmus/simulation/ProcessManager.hpp"

#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace emmus::simulation
{

Process&
ProcessManager::createProcess(
    PageCount pageCount,
    PageSize pageSize
)
{
    // Validate before allocating an identifier so a failed creation
    // does not consume a process ID.
    VirtualAddressSpace virtualAddressSpace(
        pageCount,
        pageSize
    );

    const ProcessId processId = allocateProcessId();

    auto process = std::make_unique<Process>(
        processId,
        std::move(virtualAddressSpace)
    );

    auto [iterator, inserted] =
        processes_.emplace(
            processId,
            std::move(process)
        );

    if (!inserted)
    {
        throw std::logic_error(
            "Generated duplicate process identifier."
        );
    }

    return *iterator->second;
}

Process*
ProcessManager::findProcess(
    ProcessId processId
) noexcept
{
    const auto iterator = processes_.find(processId);

    if (iterator == processes_.end())
    {
        return nullptr;
    }

    return iterator->second.get();
}

const Process*
ProcessManager::findProcess(
    ProcessId processId
) const noexcept
{
    const auto iterator = processes_.find(processId);

    if (iterator == processes_.end())
    {
        return nullptr;
    }

    return iterator->second.get();
}

bool
ProcessManager::contains(
    ProcessId processId
) const noexcept
{
    return processes_.contains(processId);
}

ProcessManager::ProcessCount
ProcessManager::processCount() const noexcept
{
    return processes_.size();
}

void
ProcessManager::clear() noexcept
{
    processes_.clear();
    nextProcessId_ = 1;
}

ProcessManager::ProcessId
ProcessManager::allocateProcessId() noexcept
{
    if (
        nextProcessId_ ==
        std::numeric_limits<std::uint64_t>::max()
    )
    {
        // This is practically unreachable under normal simulation use.
        // The caller cannot receive an invalid identifier.
        std::terminate();
    }

    return ProcessId(nextProcessId_++);
}

} // namespace emmus::simulation