#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "emmus/simulation/Process.hpp"

namespace emmus::simulation
{

/**
 * Owns and creates simulated processes.
 *
 * ProcessManager is responsible for:
 *   - generating unique process identifiers
 *   - validating process-creation input
 *   - owning the lifetime of created processes
 *   - providing access to existing processes
 *
 * It does not manage pages, frames, page tables, or physical memory.
 */
class ProcessManager
{
public:
    using ProcessId = memory::identifiers::ProcessId;
    using ProcessCount = std::size_t;
    using PageCount = VirtualAddressSpace::PageCount;
    using PageSize = VirtualAddressSpace::PageSize;

    ProcessManager() = default;

    Process& createProcess(
        PageCount pageCount,
        PageSize pageSize
    );

    [[nodiscard]] Process* findProcess(
        ProcessId processId
    ) noexcept;

    [[nodiscard]] const Process* findProcess(
        ProcessId processId
    ) const noexcept;

    [[nodiscard]] bool contains(
        ProcessId processId
    ) const noexcept;

    [[nodiscard]] ProcessCount processCount() const noexcept;

    void clear() noexcept;

private:
    ProcessId allocateProcessId() noexcept;

    std::unordered_map<
        ProcessId,
        std::unique_ptr<Process>
    > processes_;

    std::uint64_t nextProcessId_{1};
};

} // namespace emmus::simulation