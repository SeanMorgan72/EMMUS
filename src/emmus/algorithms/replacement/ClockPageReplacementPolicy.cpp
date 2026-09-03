#include "emmus/algorithms/replacement/ClockPageReplacementPolicy.hpp"

#include <algorithm>
#include <chrono>

namespace emmus::algorithms::replacement
{

void ClockPageReplacementPolicy::pageLoaded(
    PageId pageId,
    FrameId frameId
)
{
    if (containsPageOrFrame(pageId, frameId))
    {
        return;
    }

    residencyCollection_.push_back(
        ResidencyEntry{
            pageId,
            frameId,
            true
        }
    );

    if (residencyCollection_.size() == 1U)
    {
        clockHand_ = 0U;
    }
}


void ClockPageReplacementPolicy::pageAccessed(
    PageId pageId,
    FrameId frameId
)
{
    const auto iterator =
        find(pageId, frameId);

    if (iterator == residencyCollection_.end())
    {
        return;
    }

    iterator->referenceBit = true;
}


void ClockPageReplacementPolicy::pageRemoved(
    PageId pageId,
    FrameId frameId
)
{
    const auto iterator =
        find(pageId, frameId);

    if (iterator == residencyCollection_.end())
    {
        return;
    }

    const std::size_t removedIndex =
        static_cast<std::size_t>(
            std::distance(
                residencyCollection_.begin(),
                iterator
            )
        );

    residencyCollection_.erase(iterator);

    if (residencyCollection_.empty())
    {
        clockHand_ = 0U;
        return;
    }

    /*
     * When an entry before the current clock-hand position is
     * removed, the hand's numeric index must move backward so that
     * it continues to identify the same logical entry.
     */
    if (removedIndex < clockHand_)
    {
        --clockHand_;
    }
    else if (removedIndex == clockHand_)
    {
        /*
         * After removing the entry currently under the hand,
         * the next entry now occupies the same index.
         *
         * If the removed entry was the final element, wrap to zero.
         */
        if (clockHand_ >= residencyCollection_.size())
        {
            clockHand_ = 0U;
        }
    }
}


std::optional<ClockPageReplacementPolicy::FrameId>
ClockPageReplacementPolicy::chooseVictim()
{
    const auto startTime =
        std::chrono::steady_clock::now();

    if (residencyCollection_.empty())
    {
        return std::nullopt;
    }

    /*
     * The Clock algorithm must eventually find a clear reference bit.
     *
     * If every reference bit is initially set, the first complete
     * traversal clears all bits. The next inspected entry is then
     * eligible for selection.
     *
     * The traversal limit prevents an accidental infinite loop if
     * policy state is ever corrupted.
     */
    const std::size_t entryCount =
        residencyCollection_.size();

    std::size_t inspectedEntries{0U};

    while (inspectedEntries < entryCount * 2U)
    {
        ResidencyEntry& entry =
            residencyCollection_[clockHand_];

        if (!entry.referenceBit)
        {
            const FrameId victim =
                entry.frameId;

            /*
             * Advance the clock hand so that the next replacement
             * decision begins with the entry following the selected
             * victim.
             *
             * The victim remains tracked until pageRemoved().
             */
            advanceClockHand();

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

        /*
         * Second chance:
         *
         * The page has been referenced since the previous inspection.
         * Clear the bit and allow another resident entry to be examined.
         */
        entry.referenceBit = false;

        advanceClockHand();

        ++inspectedEntries;
    }

    /*
     * This should be unreachable for a consistent Clock state:
     * after at most one complete pass, all inspected entries have
     * cleared reference bits.
     *
     * Return no victim rather than risking undefined behavior if
     * an unexpected internal state is ever encountered.
     */
    return std::nullopt;
}


void ClockPageReplacementPolicy::reset()
{
    residencyCollection_.clear();

    clockHand_ = 0U;

    statistics_.reset();
}


const ClockPageReplacementPolicy::Statistics&
ClockPageReplacementPolicy::statistics() const noexcept
{
    return statistics_;
}


ClockPageReplacementPolicy::ResidencyCollection::iterator
ClockPageReplacementPolicy::find(
    PageId pageId,
    FrameId frameId
) noexcept
{
    return std::find_if(
        residencyCollection_.begin(),
        residencyCollection_.end(),
        [pageId, frameId](const ResidencyEntry& entry)
        {
            return entry.pageId == pageId
                && entry.frameId == frameId;
        }
    );
}


ClockPageReplacementPolicy::ResidencyCollection::const_iterator
ClockPageReplacementPolicy::find(
    PageId pageId,
    FrameId frameId
) const noexcept
{
    return std::find_if(
        residencyCollection_.begin(),
        residencyCollection_.end(),
        [pageId, frameId](const ResidencyEntry& entry)
        {
            return entry.pageId == pageId
                && entry.frameId == frameId;
        }
    );
}


bool ClockPageReplacementPolicy::containsPageOrFrame(
    PageId pageId,
    FrameId frameId
) const noexcept
{
    return std::any_of(
        residencyCollection_.begin(),
        residencyCollection_.end(),
        [pageId, frameId](const ResidencyEntry& entry)
        {
            return entry.pageId == pageId
                || entry.frameId == frameId;
        }
    );
}


void ClockPageReplacementPolicy::advanceClockHand() noexcept
{
    ++clockHand_;

    if (clockHand_ >= residencyCollection_.size())
    {
        clockHand_ = 0U;
    }
}

} // namespace emmus::algorithms::replacement