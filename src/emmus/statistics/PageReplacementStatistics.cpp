#include "emmus/statistics/PageReplacementStatistics.hpp"

namespace emmus::statistics
{

void PageReplacementStatistics::recordReplacement() noexcept
{
    ++replacementCount_;
}


void PageReplacementStatistics::recordDirtyEviction() noexcept
{
    ++dirtyEvictionCount_;
}


void PageReplacementStatistics::recordExecutionTime(
    Duration duration
) noexcept
{
    totalExecutionTime_ += duration;
}


PageReplacementStatistics::Counter
PageReplacementStatistics::replacementCount() const noexcept
{
    return replacementCount_;
}


PageReplacementStatistics::Counter
PageReplacementStatistics::dirtyEvictionCount() const noexcept
{
    return dirtyEvictionCount_;
}


PageReplacementStatistics::Duration
PageReplacementStatistics::totalExecutionTime() const noexcept
{
    return totalExecutionTime_;
}


double
PageReplacementStatistics::averageExecutionTimeMilliseconds() const noexcept
{
    if (replacementCount_ == 0)
    {
        return 0.0;
    }


    const std::chrono::duration<double, std::milli> totalMilliseconds =
        totalExecutionTime_;


    return totalMilliseconds.count() /
           static_cast<double>(replacementCount_);
}


void PageReplacementStatistics::reset() noexcept
{
    replacementCount_ = 0;

    dirtyEvictionCount_ = 0;

    totalExecutionTime_ = Duration::zero();
}

} // namespace emmus::statistics