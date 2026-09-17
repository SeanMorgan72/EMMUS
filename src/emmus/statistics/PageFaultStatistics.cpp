#include "emmus/statistics/PageFaultStatistics.hpp"

namespace emmus::statistics
{

void PageFaultStatistics::recordAccess(ProcessId processId) noexcept
{
    auto& stats = processStatistics_[processId];
    ++stats.accessCount;
    ++totalAccessCount_;
}

void PageFaultStatistics::recordPageFault(ProcessId processId) noexcept
{
    auto& stats = processStatistics_[processId];
    ++stats.pageFaultCount;
    ++totalPageFaultCount_;
}

PageFaultStatistics::Counter
PageFaultStatistics::totalAccessCount() const noexcept
{
    return totalAccessCount_;
}

PageFaultStatistics::Counter
PageFaultStatistics::totalPageFaultCount() const noexcept
{
    return totalPageFaultCount_;
}

std::size_t PageFaultStatistics::processCount() const noexcept
{
    return processStatistics_.size();
}

PageFaultStatistics::Counter
PageFaultStatistics::accessCount(ProcessId processId) const noexcept
{
    const auto iterator = processStatistics_.find(processId);
    if (iterator == processStatistics_.end())
    {
        return 0U;
    }

    return iterator->second.accessCount;
}

PageFaultStatistics::Counter
PageFaultStatistics::pageFaultCount(ProcessId processId) const noexcept
{
    const auto iterator = processStatistics_.find(processId);
    if (iterator == processStatistics_.end())
    {
        return 0U;
    }

    return iterator->second.pageFaultCount;
}

double PageFaultStatistics::faultRate() const noexcept
{
    if (totalAccessCount_ == 0U)
    {
        return 0.0;
    }

    return static_cast<double>(totalPageFaultCount_) /
           static_cast<double>(totalAccessCount_);
}

double PageFaultStatistics::faultRate(ProcessId processId) const noexcept
{
    const auto iterator = processStatistics_.find(processId);
    if (iterator == processStatistics_.end())
    {
        return 0.0;
    }

    if (iterator->second.accessCount == 0U)
    {
        return 0.0;
    }

    return static_cast<double>(iterator->second.pageFaultCount) /
           static_cast<double>(iterator->second.accessCount);
}

void PageFaultStatistics::reset() noexcept
{
    processStatistics_.clear();
    totalAccessCount_ = 0U;
    totalPageFaultCount_ = 0U;
}

} // namespace emmus::statistics
