#pragma once

#include <chrono>
#include <cstdint>

namespace emmus::statistics
{

/**
 * @brief Collects statistics for page-replacement operations.
 *
 * PageReplacementStatistics records the number of page replacements,
 * the number of dirty-page evictions, and the accumulated execution
 * time associated with replacement operations.
 *
 * This class is responsible only for statistics collection and
 * calculation. It does not perform page replacement, victim
 * selection, physical-memory management, or page-table management.
 */
class PageReplacementStatistics final
{
public:

    using Counter = std::uint64_t;

    using Duration = std::chrono::nanoseconds;


    /**
     * @brief Constructs an empty statistics collection.
     *
     * All counters and accumulated durations are initialized to zero.
     */
    PageReplacementStatistics() noexcept = default;


    PageReplacementStatistics(
        const PageReplacementStatistics&
    ) = default;

    PageReplacementStatistics(
        PageReplacementStatistics&&
    ) noexcept = default;

    PageReplacementStatistics& operator=(
        const PageReplacementStatistics&
    ) = default;

    PageReplacementStatistics& operator=(
        PageReplacementStatistics&&
    ) noexcept = default;

    ~PageReplacementStatistics() = default;


    /**
     * @brief Records one page replacement.
     *
     * Increments the page-replacement counter by one.
     */
    void recordReplacement() noexcept;


    /**
     * @brief Records one dirty-page eviction.
     *
     * Increments the dirty-page eviction counter by one.
     */
    void recordDirtyEviction() noexcept;


    /**
     * @brief Records execution time for one replacement operation.
     *
     * The supplied duration is added to the accumulated replacement
     * execution time.
     *
     * @param duration Execution duration to add.
     */
    void recordExecutionTime(
        Duration duration
    ) noexcept;


    /**
     * @brief Returns the number of recorded page replacements.
     */
    [[nodiscard]]
    Counter replacementCount() const noexcept;


    /**
     * @brief Returns the number of recorded dirty-page evictions.
     */
    [[nodiscard]]
    Counter dirtyEvictionCount() const noexcept;


    /**
     * @brief Returns the accumulated replacement execution time.
     */
    [[nodiscard]]
    Duration totalExecutionTime() const noexcept;


    /**
     * @brief Calculates the average replacement execution time.
     *
     * The average is calculated from the accumulated execution time
     * divided by the number of recorded replacements.
     *
     * @return Average replacement execution time in milliseconds.
     *
     * @return 0.0 when no replacements have been recorded.
     */
    [[nodiscard]]
    double averageExecutionTimeMilliseconds() const noexcept;


    /**
     * @brief Resets all collected statistics.
     *
     * After reset(), all counters and accumulated execution time
     * return to their initial zero state.
     */
    void reset() noexcept;


private:

    Counter replacementCount_{0};

    Counter dirtyEvictionCount_{0};

    Duration totalExecutionTime_{Duration::zero()};
};

} // namespace emmus::statistics