#pragma once

#include <cstddef>
#include <vector>

#include "emmus/application/MemoryAccessExecutionStatistics.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessResult.hpp"

namespace emmus::application
{

/**
 * @brief Associates a submitted memory access with its execution result.
 *
 * Results are stored in exactly the order in which the corresponding
 * MemoryAccess objects were submitted to the MMU.
 */
struct MemoryAccessExecutionRecord
{
    memory::access::MemoryAccess access;
    memory::access::MemoryAccessResult result;

    bool operator==(const MemoryAccessExecutionRecord&) const = default;
};


/**
 * @brief Result of executing an ordered memory-access sequence.
 *
 * The result preserves every submitted access and its corresponding MMU
 * result, followed by aggregate statistics for the execution.
 */
class MemoryAccessExecutionResult final
{
public:

    using Record = MemoryAccessExecutionRecord;
    using Records = std::vector<Record>;
    using ConstIterator = Records::const_iterator;


    MemoryAccessExecutionResult() = default;


    /**
     * @brief Returns all access/result records in execution order.
     */
    [[nodiscard]]
    const Records& records() const noexcept
    {
        return records_;
    }


    /**
     * @brief Returns the number of executed accesses.
     */
    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return records_.size();
    }


    /**
     * @brief Returns whether no accesses were executed.
     */
    [[nodiscard]]
    bool empty() const noexcept
    {
        return records_.empty();
    }


    [[nodiscard]]
    ConstIterator begin() const noexcept
    {
        return records_.begin();
    }


    [[nodiscard]]
    ConstIterator end() const noexcept
    {
        return records_.end();
    }


    /**
     * @brief Returns aggregate execution statistics.
     */
    [[nodiscard]]
    const MemoryAccessExecutionStatistics& statistics() const noexcept
    {
        return statistics_;
    }


private:

    friend class MemoryAccessExecutor;

    Records records_;
    MemoryAccessExecutionStatistics statistics_;
};

} // namespace emmus::application