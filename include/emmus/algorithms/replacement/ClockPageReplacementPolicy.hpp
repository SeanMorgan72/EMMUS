#pragma once

#include <cstddef>
#include <vector>

#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/statistics/PageReplacementStatistics.hpp"

namespace emmus::algorithms::replacement
{

/**
 * @brief Clock page-replacement policy.
 *
 * Clock implements the second-chance page-replacement algorithm using
 * a circular collection of resident page/frame associations.
 *
 * Each tracked entry maintains a reference bit:
 *
 * - reference bit == true  -> the entry receives a second chance;
 * - reference bit == false -> the entry is eligible for selection.
 *
 * The clock hand identifies the next entry to inspect.
 *
 * During victim selection:
 *
 * 1. Inspect the entry at the clock hand.
 * 2. If its reference bit is false, select it as the victim.
 * 3. If its reference bit is true, clear the bit and advance the hand.
 * 4. Continue until an entry with a cleared reference bit is found.
 *
 * The selected victim remains tracked until pageRemoved() is called.
 *
 * The policy maintains replacement state only. It does not own or
 * modify Page or Frame objects, perform physical-memory management,
 * update page tables, or perform the actual eviction.
 */
class ClockPageReplacementPolicy final
    : public IPageReplacementPolicy
{
public:

    using Statistics =
        emmus::statistics::PageReplacementStatistics;


    /**
     * @brief Constructs an empty Clock replacement policy.
     */
    ClockPageReplacementPolicy() = default;


    ClockPageReplacementPolicy(
        const ClockPageReplacementPolicy&
    ) = delete;


    ClockPageReplacementPolicy& operator=(
        const ClockPageReplacementPolicy&
    ) = delete;


    ClockPageReplacementPolicy(
        ClockPageReplacementPolicy&&
    ) noexcept = default;


    ClockPageReplacementPolicy& operator=(
        ClockPageReplacementPolicy&&
    ) noexcept = default;


    ~ClockPageReplacementPolicy() override = default;


    /**
     * @brief Records a newly loaded page/frame association.
     *
     * A newly loaded entry is appended to the circular residency
     * collection and begins with its reference bit set.
     *
     * Duplicate page/frame associations are ignored.
     * A page already associated with another frame is also ignored,
     * as is a frame already associated with another page.
     *
     * @param pageId Page that became resident.
     * @param frameId Frame containing the page.
     */
    void pageLoaded(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Marks a resident page/frame as referenced.
     *
     * A valid access sets the corresponding reference bit to true.
     *
     * Access notifications for untracked or mismatched
     * page/frame associations are ignored.
     *
     * @param pageId Accessed page.
     * @param frameId Frame containing the page.
     */
    void pageAccessed(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Removes a page/frame association from Clock tracking.
     *
     * The clock hand is adjusted so that removal of an entry cannot
     * invalidate the circular traversal position.
     *
     * Mismatched or untracked notifications are ignored.
     *
     * @param pageId Page being removed.
     * @param frameId Frame previously containing the page.
     */
    void pageRemoved(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Selects a victim using the Clock second-chance algorithm.
     *
     * Entries with their reference bit set receive a second chance:
     * the bit is cleared and the clock hand advances.
     *
     * The first entry encountered with a cleared reference bit is
     * selected as the victim.
     *
     * The selected entry remains tracked until pageRemoved() is
     * subsequently called.
     *
     * @return The selected victim FrameId.
     * @return std::nullopt when no resident frame is tracked.
     */
    [[nodiscard]]
    std::optional<FrameId> chooseVictim() override;


    /**
     * @brief Resets all Clock state and replacement statistics.
     */
    void reset() override;


    /**
     * @brief Returns the collected replacement statistics.
     */
    [[nodiscard]]
    const Statistics& statistics() const noexcept;


private:

    /**
     * @brief Represents one tracked resident page/frame association.
     */
    struct ResidencyEntry
    {
        PageId pageId;
        FrameId frameId;
        bool referenceBit;
    };


    using ResidencyCollection =
        std::vector<ResidencyEntry>;


    /**
     * @brief Finds an exact page/frame association.
     *
     * @return Iterator to the matching entry, or end() when not found.
     */
    [[nodiscard]]
    ResidencyCollection::iterator find(
        PageId pageId,
        FrameId frameId
    ) noexcept;


    /**
     * @brief Finds an exact page/frame association.
     */
    [[nodiscard]]
    ResidencyCollection::const_iterator find(
        PageId pageId,
        FrameId frameId
    ) const noexcept;


    /**
     * @brief Determines whether a page or frame is already tracked.
     */
    [[nodiscard]]
    bool containsPageOrFrame(
        PageId pageId,
        FrameId frameId
    ) const noexcept;


    /**
     * @brief Advances the clock hand by one position.
     *
     * The collection must not be empty when this function is called.
     */
    void advanceClockHand() noexcept;


    ResidencyCollection residencyCollection_;

    std::size_t clockHand_{0U};

    Statistics statistics_;
};

} // namespace emmus::algorithms::replacement