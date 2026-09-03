#include "emmus/algorithms/replacement/OptimalPageReplacementPolicy.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>

namespace emmus::algorithms::replacement
{

void OptimalPageReplacementPolicy::setReferenceSequence(
    ReferenceSequence referenceSequence
)
{
    referenceSequence_ =
        std::move(referenceSequence);

    referencePosition_ = 0U;
}


const OptimalPageReplacementPolicy::ReferenceSequence&
OptimalPageReplacementPolicy::referenceSequence() const noexcept
{
    return referenceSequence_;
}


void OptimalPageReplacementPolicy::pageLoaded(
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
            nextResidencyOrder_
        }
    );

    ++nextResidencyOrder_;
}


void OptimalPageReplacementPolicy::pageAccessed(
    PageId pageId,
    FrameId frameId
)
{
    /*
     * Optimal must remain synchronized with the complete workload.
     *
     * Unlike historical policies, an access to a currently
     * non-resident page still represents a real position in the
     * reference sequence and therefore advances the future-workload
     * position.
     */
    advanceReferencePosition(pageId);

    /*
     * Residency state is only modified for an exact mapping.
     *
     * Optimal does not need an access timestamp or reference bit.
     */
    const auto iterator =
        find(pageId, frameId);

    if (iterator == residencyCollection_.end())
    {
        return;
    }
}


void OptimalPageReplacementPolicy::pageRemoved(
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

    residencyCollection_.erase(iterator);
}


std::optional<OptimalPageReplacementPolicy::FrameId>
OptimalPageReplacementPolicy::chooseVictim()
{
    const auto startTime =
        std::chrono::steady_clock::now();

    if (residencyCollection_.empty())
    {
        return std::nullopt;
    }

    const ResidencyEntry* selectedEntry =
        &residencyCollection_.front();

    std::optional<std::size_t> selectedNextUse =
        nextUse(selectedEntry->pageId);

    for (
        auto iterator =
            std::next(residencyCollection_.begin());
        iterator != residencyCollection_.end();
        ++iterator
    )
    {
        const auto candidateNextUse =
            nextUse(iterator->pageId);

        /*
         * A page that is never referenced again is the optimal victim.
         *
         * Once such a candidate is found, no finite future-use distance
         * can make another candidate preferable.
         */
        if (
            !candidateNextUse.has_value()
            && selectedNextUse.has_value()
        )
        {
            selectedEntry =
                &(*iterator);

            selectedNextUse =
                candidateNextUse;

            continue;
        }

        if (
            candidateNextUse.has_value()
            && selectedNextUse.has_value()
        )
        {
            /*
             * Select the page whose next reference occurs farther
             * in the future.
             *
             * Equal positions are intentionally left unchanged so
             * that residency order provides deterministic behavior.
             */
            if (
                *candidateNextUse
                > *selectedNextUse
            )
            {
                selectedEntry =
                    &(*iterator);

                selectedNextUse =
                    candidateNextUse;
            }

            continue;
        }

        /*
         * If both pages have no future use, retain the earlier
         * residency entry as the deterministic tie-breaker.
         */
    }

    const FrameId victim =
        selectedEntry->frameId;

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


void OptimalPageReplacementPolicy::reset()
{
    residencyCollection_.clear();

    referenceSequence_.clear();

    referencePosition_ = 0U;

    nextResidencyOrder_ = 0U;

    statistics_.reset();
}


const OptimalPageReplacementPolicy::Statistics&
OptimalPageReplacementPolicy::statistics() const noexcept
{
    return statistics_;
}


OptimalPageReplacementPolicy::ResidencyCollection::iterator
OptimalPageReplacementPolicy::find(
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


OptimalPageReplacementPolicy::ResidencyCollection::const_iterator
OptimalPageReplacementPolicy::find(
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


bool OptimalPageReplacementPolicy::containsPageOrFrame(
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


std::optional<std::size_t>
OptimalPageReplacementPolicy::nextUse(
    PageId pageId
) const noexcept
{
    const auto iterator =
        std::find(
            referenceSequence_.begin()
                + static_cast<
                    std::ptrdiff_t
                  >(referencePosition_),
            referenceSequence_.end(),
            pageId
        );

    if (iterator == referenceSequence_.end())
    {
        return std::nullopt;
    }

    return static_cast<std::size_t>(
        std::distance(
            referenceSequence_.begin(),
            iterator
        )
    );
}


void OptimalPageReplacementPolicy::advanceReferencePosition(
    PageId pageId
) noexcept
{
    if (
        referencePosition_
        >= referenceSequence_.size()
    )
    {
        return;
    }

    const auto iterator =
        std::find(
            referenceSequence_.begin()
                + static_cast<
                    std::ptrdiff_t
                  >(referencePosition_),
            referenceSequence_.end(),
            pageId
        );

    if (iterator == referenceSequence_.end())
    {
        return;
    }

    referencePosition_ =
        static_cast<std::size_t>(
            std::distance(
                referenceSequence_.begin(),
                iterator
            )
        ) + 1U;
}

} // namespace emmus::algorithms::replacement