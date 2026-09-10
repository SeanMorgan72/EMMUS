#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace emmus::application
{

/**
 * @brief Statistics collected while executing a memory-access sequence.
 *
 * MemoryAccessExecutionStatistics describes the outcome of one execution
 * sequence. It is independent of any particular page-replacement policy.
 *
 * Replacement-specific timing remains the responsibility of
 * PageReplacementStatistics.
 */
class MemoryAccessExecutionStatistics final
{
public:

    using Counter = std::uint64_t;
    using Duration = std::chrono::nanoseconds;


    /**
     * @brief Returns the number of memory accesses submitted for execution.
     */
    [[nodiscard]]
    Counter memoryAccessCount() const noexcept
    {
        return memoryAccessCount_;
    }


    /**
     * @brief Returns the number of successful memory accesses.
     */
    [[nodiscard]]
    Counter successfulAccessCount() const noexcept
    {
        return successfulAccessCount_;
    }


    /**
     * @brief Returns the number of failed memory accesses.
     */
    [[nodiscard]]
    Counter failedAccessCount() const noexcept
    {
        return failedAccessCount_;
    }


    /**
     * @brief Returns the number of page faults reported by the MMU.
     */
    [[nodiscard]]
    Counter pageFaultCount() const noexcept
    {
        return pageFaultCount_;
    }


    /**
     * @brief Returns the number of page replacements reported by the MMU.
     */
    [[nodiscard]]
    Counter pageReplacementCount() const noexcept
    {
        return pageReplacementCount_;
    }


    /**
     * @brief Returns the number of dirty-page evictions.
     */
    [[nodiscard]]
    Counter dirtyEvictionCount() const noexcept
    {
        return dirtyEvictionCount_;
    }


    /**
     * @brief Returns the total execution time for the sequence.
     */
    [[nodiscard]]
    Duration totalExecutionTime() const noexcept
    {
        return totalExecutionTime_;
    }


    /**
     * @brief Returns the page-fault rate as a fraction in [0, 1].
     */
    [[nodiscard]]
    double pageFaultRate() const noexcept
    {
        if (memoryAccessCount_ == 0U)
        {
            return 0.0;
        }

        return static_cast<double>(pageFaultCount_) /
               static_cast<double>(memoryAccessCount_);
    }


private:

    friend class MemoryAccessExecutor;

    Counter memoryAccessCount_{0U};
    Counter successfulAccessCount_{0U};
    Counter failedAccessCount_{0U};
    Counter pageFaultCount_{0U};
    Counter pageReplacementCount_{0U};
    Counter dirtyEvictionCount_{0U};

    Duration totalExecutionTime_{Duration::zero()};
};

} // namespace emmus::application