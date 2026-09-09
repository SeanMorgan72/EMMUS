#pragma once

#include <cstddef>
#include <cstdint>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::simulation
{

/**
 * Represents the lifecycle state of a simulated process.
 *
 * US-1001 only requires process creation. Execution/scheduling states
 * therefore remain intentionally minimal until the simulation engine
 * introduces the corresponding behavior.
 */
enum class ProcessState
{
    Created,
    Terminated
};

/**
 * Describes the virtual address space belonging to a process.
 *
 * This is process-local address-space metadata. It does not own or
 * manipulate
 * physical memory.
 */
class VirtualAddressSpace
{
public:
    using PageCount = std::uint64_t;
    using PageSize = std::uint64_t;

    VirtualAddressSpace(PageCount pageCount, PageSize pageSize);

    [[nodiscard]] PageCount pageCount() const noexcept;
    [[nodiscard]] PageSize pageSize() const noexcept;
    [[nodiscard]] std::uint64_t sizeInBytes() const noexcept;

private:
    PageCount pageCount_;
    PageSize pageSize_;
};

/**
 * Represents a simulated process.
 *
 * The process owns its identity and virtual-address-space metadata.
 * Page-table and workload integration are deliberately left at their
 * appropriate subsystem boundaries until those components are integrated
 * by the simulation layer.
 */
class Process
{
public:
    using ProcessId = memory::identifiers::ProcessId;

    Process(
        ProcessId processId,
        VirtualAddressSpace virtualAddressSpace
    );

    [[nodiscard]] ProcessId processId() const noexcept;

    [[nodiscard]] const VirtualAddressSpace&
    virtualAddressSpace() const noexcept;

    [[nodiscard]] ProcessState state() const noexcept;

    void terminate() noexcept;

private:
    ProcessId processId_;
    VirtualAddressSpace virtualAddressSpace_;
    ProcessState state_;
};

} // namespace emmus::simulation