#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/statistics/PageReplacementStatistics.hpp"

namespace emmus::algorithms::replacement
{

/**
 * @brief Least Recently Used (LRU) page replacement policy.
 *
 * LRU selects the page/frame that has been accessed least recently
 * as the replacement victim.
 *
 * The policy maintains replacement state only. It does not own
 * Page or Frame objects, perform physical-memory management,
 * update page tables, or perform the actual eviction.
 *
 * Residency entries are maintained in access-order:
 *
 * - front  = least recently used (LRU);
 * - back   = most recently used (MRU).
 *
 * A page becomes MRU when it is loaded or accessed.
 *
 * chooseVictim() selects the LRU frame but does not remove it from
 * the policy's tracking state. The entry remains tracked until the
 * caller reports the actual removal through pageRemoved().
 */
class LRUPageReplacementPolicy final : public IPageReplacementPolicy
{
public:

    using Statistics = emmus::statistics::PageReplacementStatistics;


    /**
     * @brief Constructs an empty LRU replacement policy.
     */
    LRUPageReplacementPolicy() = default;


    LRUPageReplacementPolicy(
        const LRUPageReplacementPolicy&
    ) = delete;

    LRUPageReplacementPolicy& operator=(
        const LRUPageReplacementPolicy&
    ) = delete;

    LRUPageReplacementPolicy(
        LRUPageReplacementPolicy&&
    ) noexcept = default;

    LRUPageReplacementPolicy& operator=(
        LRUPageReplacementPolicy&&
    ) noexcept = default;

    ~LRUPageReplacementPolicy() override = default;


    /**
     * @brief Records a newly resident page/frame association.
     *
     * A successfully tracked page is placed at the MRU position.
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
     * @brief Records access to a resident page/frame association.
     *
     * The accessed entry is moved to the MRU position.
     *
     * An access notification for an untracked or mismatched
     * page/frame association is ignored.
     *
     * @param pageId Accessed page.
     * @param frameId Frame containing the page.
     */
    void pageAccessed(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Removes a page/frame association from policy tracking.
     *
     * Only the exact tracked page/frame association is removed.
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
     * @brief Selects the least recently used frame.
     *
     * The selected entry remains tracked until pageRemoved() is
     * subsequently called by the replacement subsystem.
     *
     * @return The LRU FrameId when at least one entry is tracked.
     *
     * @return std::nullopt when no eligible frame is tracked.
     */
    [[nodiscard]]
    std::optional<FrameId> chooseVictim() override;


    /**
     * @brief Resets residency tracking and replacement statistics.
     */
    void reset() override;


    /**
     * @brief Returns the replacement statistics.
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
    };


    /**
     * @brief Ordered residency list.
     *
     * The front entry is the LRU entry and the back entry is
     * the MRU entry.
     */
    using ResidencyList = std::list<ResidencyEntry>;

    using ResidencyIterator = ResidencyList::iterator;


    /**
     * @brief Determines whether an exact page/frame association
     *        is already tracked.
     */
    [[nodiscard]]
    bool contains(
        PageId pageId,
        FrameId frameId
    ) const noexcept;


    ResidencyList residencyList_;


    /*
     * Page and frame lookup tables provide constant-time lookup
     * while residencyList_ maintains the ordering required by LRU.
     */
    std::unordered_map<PageId, ResidencyIterator> pageLookup_;

    std::unordered_map<FrameId, ResidencyIterator> frameLookup_;


    Statistics statistics_;
};

} // namespace emmus::algorithms::replacement