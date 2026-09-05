#pragma once

#include <cstddef>

#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessResult.hpp"
#include "emmus/memory/virtual/Page.hpp"

namespace emmus::memory::mmu
{

/**
 * @brief Interface for the Enhanced Memory Management Unit.
 *
 * The memory-management unit coordinates virtual-memory accesses,
 * page-table mappings, physical-frame allocation, page-fault handling,
 * page replacement, and page state updates.
 *
 * The MMU does not delegate page ownership to a ProcessManager.  Instead,
 * pages required by the simulated memory-access workload are explicitly
 * registered with the MMU.
 */
class IMemoryManagementUnit
{
public:

    using MemoryAccess =
        emmus::memory::access::MemoryAccess;

    using MemoryAccessResult =
        emmus::memory::access::MemoryAccessResult;

    using Page =
        emmus::memory::virtual_memory::Page;

    /**
     * @brief Virtual destructor.
     */
    virtual ~IMemoryManagementUnit() = default;


    /**
     * @brief Registers a virtual page with the MMU.
     *
     * A registered page becomes available for subsequent memory accesses.
     * The page's process identifier and page identifier are used together
     * to maintain process isolation.
     *
     * Registration does not make the page resident and does not allocate
     * a physical frame.
     *
     * @param page Page object to register.
     *
     * @return true when the page was registered.
     * @return false when a page with the same process/page identity is
     *         already registered.
     */
    [[nodiscard]]
    virtual bool registerPage(
        Page page
    ) = 0;


    /**
     * @brief Processes one virtual-memory access.
     *
     * The MMU validates the access, determines the requested page,
     * detects whether a page fault occurs, allocates a free frame or
     * selects a replacement victim when necessary, updates mappings
     * and page state, and returns a structured access result.
     *
     * A successful write access marks the accessed page dirty.
     * A successful read or write access marks the accessed page
     * referenced.
     *
     * @param access Memory access request to process.
     *
     * @return Structured result describing the outcome of the access.
     */
    [[nodiscard]]
    virtual MemoryAccessResult access(
        const MemoryAccess& access
    ) = 0;


    /**
     * @brief Resets MMU runtime state.
     *
     * Reset clears MMU-managed residency/mapping state and replacement
     * state while retaining the registered virtual-page definitions.
     */
    virtual void reset() = 0;
};

} // namespace emmus::memory::mmu