#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"

#include <algorithm>
#include <chrono>

namespace emmus::algorithms::replacement
{

void FIFOPageReplacementPolicy::pageLoaded(
    PageId pageId,
    FrameId frameId
)
{
    if (contains(pageId, frameId))
    {
        return;
    }

    const auto existingEntry =
        std::find_if(
            residencyQueue_.begin(),
            residencyQueue_.end(),
            [pageId, frameId](const ResidencyEntry& entry)
            {
                return entry.pageId == pageId
                    || entry.frameId == frameId;
            }
        );

    if (existingEntry != residencyQueue_.end())
    {
        return;
    }

    residencyQueue_.push_back(
        ResidencyEntry{
            pageId,
            frameId
        }
    );
}


void FIFOPageReplacementPolicy::pageAccessed(
    PageId,
    FrameId
)
{
    // FIFO ordering is determined exclusively by load order.
    //
    // A page access must never move an entry within the queue.
}


void FIFOPageReplacementPolicy::pageRemoved(
    PageId pageId,
    FrameId frameId
)
{
    const auto iterator =
        std::find_if(
            residencyQueue_.begin(),
            residencyQueue_.end(),
            [pageId, frameId](const ResidencyEntry& entry)
            {
                return entry.pageId == pageId
                    && entry.frameId == frameId;
            }
        );

    if (iterator == residencyQueue_.end())
    {
        return;
    }

    residencyQueue_.erase(iterator);
}


std::optional<FIFOPageReplacementPolicy::FrameId>
FIFOPageReplacementPolicy::chooseVictim()
{
    const auto startTime =
        std::chrono::steady_clock::now();

    if (residencyQueue_.empty())
    {
        return std::nullopt;
    }

    const FrameId victim =
        residencyQueue_.front().frameId;

    const auto endTime =
        std::chrono::steady_clock::now();

    statistics_.recordReplacement();

    statistics_.recordExecutionTime(
        std::chrono::duration_cast<Statistics::Duration>(
            endTime - startTime
        )
    );

    return victim;
}


void FIFOPageReplacementPolicy::reset()
{
    residencyQueue_.clear();

    statistics_.reset();
}


const FIFOPageReplacementPolicy::Statistics&
FIFOPageReplacementPolicy::statistics() const noexcept
{
    return statistics_;
}


bool FIFOPageReplacementPolicy::contains(
    PageId pageId,
    FrameId frameId
) const noexcept
{
    return std::any_of(
        residencyQueue_.begin(),
        residencyQueue_.end(),
        [pageId, frameId](const ResidencyEntry& entry)
        {
            return entry.pageId == pageId
                && entry.frameId == frameId;
        }
    );
}

} // namespace emmus::algorithms::replacement