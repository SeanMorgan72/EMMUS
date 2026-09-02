#include "emmus/algorithms/replacement/LRUPageReplacementPolicy.hpp"

#include <chrono>
#include <iterator>

namespace emmus::algorithms::replacement
{

void LRUPageReplacementPolicy::pageLoaded(
    PageId pageId,
    FrameId frameId
)
{
    /*
     * The page/frame association is already tracked.
     */
    if (contains(pageId, frameId))
    {
        return;
    }


    /*
     * A page may only be associated with one frame.
     *
     * A frame may only contain one page.
     *
     * These checks also prevent inconsistent duplicate residency
     * notifications from corrupting the LRU ordering.
     */
    if (pageLookup_.contains(pageId) ||
        frameLookup_.contains(frameId))
    {
        return;
    }


    /*
     * Newly loaded pages are the most recently used entries.
     */
    residencyList_.push_back(
        ResidencyEntry{
            pageId,
            frameId
        }
    );


    const ResidencyIterator iterator =
        std::prev(residencyList_.end());


    pageLookup_.emplace(
        pageId,
        iterator
    );

    frameLookup_.emplace(
        frameId,
        iterator
    );
}


void LRUPageReplacementPolicy::pageAccessed(
    PageId pageId,
    FrameId frameId
)
{
    const auto pageIterator =
        pageLookup_.find(pageId);


    /*
     * Ignore accesses to pages that are not currently tracked.
     */
    if (pageIterator == pageLookup_.end())
    {
        return;
    }


    const ResidencyIterator residencyIterator =
        pageIterator->second;


    /*
     * Ignore a notification whose frame does not match the
     * frame currently associated with the page.
     */
    if (residencyIterator->frameId != frameId)
    {
        return;
    }


    /*
     * The accessed entry becomes the most recently used entry.
     *
     * std::list::splice() moves the existing node without copying
     * or invalidating the iterator stored in the lookup tables.
     */
    const ResidencyIterator lastIterator =
        std::prev(residencyList_.end());


    if (residencyIterator != lastIterator)
    {
        residencyList_.splice(
            residencyList_.end(),
            residencyList_,
            residencyIterator
        );
    }
}


void LRUPageReplacementPolicy::pageRemoved(
    PageId pageId,
    FrameId frameId
)
{
    const auto pageIterator =
        pageLookup_.find(pageId);


    /*
     * Ignore removal notifications for pages that are not tracked.
     */
    if (pageIterator == pageLookup_.end())
    {
        return;
    }


    const ResidencyIterator residencyIterator =
        pageIterator->second;


    /*
     * Only the exact page/frame association may be removed.
     */
    if (residencyIterator->frameId != frameId)
    {
        return;
    }


    /*
     * Remove both lookup entries before erasing the list node.
     */
    pageLookup_.erase(pageId);
    frameLookup_.erase(frameId);

    residencyList_.erase(residencyIterator);
}


std::optional<LRUPageReplacementPolicy::FrameId>
LRUPageReplacementPolicy::chooseVictim()
{
    const auto startTime =
        std::chrono::steady_clock::now();


    /*
     * No tracked resident frames means that there is no
     * replacement candidate.
     */
    if (residencyList_.empty())
    {
        return std::nullopt;
    }


    /*
     * The front of the list is always the least recently used
     * entry.
     */
    const FrameId victim =
        residencyList_.front().frameId;


    const auto endTime =
        std::chrono::steady_clock::now();


    /*
     * Selecting a victim constitutes one replacement decision.
     *
     * The policy does not record dirty eviction here because it
     * does not own page state and does not perform the eviction.
     */
    statistics_.recordReplacement();

    statistics_.recordExecutionTime(
        std::chrono::duration_cast<Statistics::Duration>(
            endTime - startTime
        )
    );


    /*
     * The victim remains tracked until the caller performs the
     * actual removal and reports it through pageRemoved().
     */
    return victim;
}


void LRUPageReplacementPolicy::reset()
{
    residencyList_.clear();

    pageLookup_.clear();

    frameLookup_.clear();

    statistics_.reset();
}


const LRUPageReplacementPolicy::Statistics&
LRUPageReplacementPolicy::statistics() const noexcept
{
    return statistics_;
}


bool LRUPageReplacementPolicy::contains(
    PageId pageId,
    FrameId frameId
) const noexcept
{
    const auto pageIterator =
        pageLookup_.find(pageId);


    if (pageIterator == pageLookup_.end())
    {
        return false;
    }


    return pageIterator->second->frameId == frameId;
}

} // namespace emmus::algorithms::replacement