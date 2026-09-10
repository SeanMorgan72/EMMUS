#pragma once

#include <span>
#include <vector>

#include "emmus/application/MemoryAccessExecutionResult.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/mmu/IMemoryManagementUnit.hpp"

namespace emmus::application
{

/**
 * @brief Executes an ordered sequence of memory accesses.
 *
 * MemoryAccessExecutor is responsible only for orchestration. It submits
 * each MemoryAccess to the supplied memory-management unit in the exact
 * order provided by the caller.
 *
 * The executor does not perform address translation, page-fault handling,
 * frame allocation, replacement, page-table management, or replacement
 * policy selection. Those responsibilities remain with the MMU and its
 * collaborators.
 */
class MemoryAccessExecutor final
{
public:

    using Access = memory::access::MemoryAccess;
    using MemoryManagementUnit =
        memory::mmu::IMemoryManagementUnit;


    explicit MemoryAccessExecutor(
        MemoryManagementUnit& memoryManagementUnit
    ) noexcept;


    /**
     * @brief Executes an ordered sequence of memory accesses.
     *
     * Each access is submitted exactly once and in the order supplied
     * by the caller.
     *
     * Invalid memory accesses are represented by their corresponding
     * MemoryAccessResult and do not prevent subsequent accesses from
     * being executed.
     *
     * @param accesses Ordered memory-access sequence.
     *
     * @return Results and aggregate statistics in execution order.
     */
    [[nodiscard]]
    MemoryAccessExecutionResult execute(
        std::span<const Access> accesses
    );


    /**
     * @brief Executes a vector of memory accesses.
     *
     * Convenience overload for callers that already own a vector.
     */
    [[nodiscard]]
    MemoryAccessExecutionResult execute(
        const std::vector<Access>& accesses
    );


private:

    MemoryManagementUnit& memoryManagementUnit_;
};

} // namespace emmus::application