#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>

#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/statistics/PageReplacementStatistics.hpp"

namespace emmus::algorithms::replacement
{

/**
 * @brief First-In, First-Out page-replacement policy.
 *
 * FIFO selects the oldest currently resident page/frame for replacement.
 *
 * Pages are tracked in the order in which they are loaded. Accessing a
 * resident page does not alter that order.
 *
 * The policy maintains replacement state only. It does not own or modify
 * Page or Frame objects and does not perform physical-memory management.
 */
class FIFOPageReplacementPolicy final
    : public IPageReplacementPolicy
{
public:

    using Statistics =
        emmus::statistics::PageReplacementStatistics;


    /**
     * @brief Constructs an empty FIFO replacement policy.
     */
    FIFOPageReplacementPolicy() = default;


    FIFOPageReplacementPolicy(
        const FIFOPageReplacementPolicy&
    ) = delete;


    FIFOPageReplacementPolicy& operator=(
        const FIFOPageReplacementPolicy&
    ) = delete;


    FIFOPageReplacementPolicy(
        FIFOPageReplacementPolicy&&
    ) noexcept = default;


    FIFOPageReplacementPolicy& operator=(
        FIFOPageReplacementPolicy&&
    ) noexcept = default;


    ~FIFOPageReplacementPolicy() override = default;


    /**
     * @brief Records a newly loaded page/frame.
     *
     * The frame is placed at the back of the FIFO residency order.
     *
     * If the page/frame association is already tracked, the notification
     * is ignored so that duplicate notifications cannot corrupt the FIFO
     * ordering.
     *
     * @param pageId Identifier of the loaded page.
     * @param frameId Identifier of the frame containing the page.
     */
    void pageLoaded(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Records access to a resident page/frame.
     *
     * FIFO does not modify its residency order when a page is accessed.
     *
     * @param pageId Identifier of the accessed page.
     * @param frameId Identifier of the frame containing the page.
     */
    void pageAccessed(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Removes a page/frame from FIFO tracking.
     *
     * A page/frame that is no longer resident cannot be selected as a
     * replacement victim.
     *
     * @param pageId Identifier of the removed page.
     * @param frameId Identifier of the released frame.
     */
    void pageRemoved(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Selects the oldest currently tracked frame.
     *
     * The selected frame remains tracked until pageRemoved() is called.
     * This allows the memory-management subsystem to perform eviction
     * before notifying the policy that the frame has been released.
     *
     * @return The oldest resident frame.
     * @return std::nullopt when no resident frames are tracked.
     */
    [[nodiscard]]
    std::optional<FrameId> chooseVictim() override;


    /**
     * @brief Clears all FIFO residency state and statistics.
     */
    void reset() override;


    /**
     * @brief Returns the collected replacement statistics.
     */
    [[nodiscard]]
    const Statistics& statistics() const noexcept;


private:

    struct ResidencyEntry
    {
        PageId pageId;
        FrameId frameId;
    };


    using ResidencyQueue =
        std::deque<ResidencyEntry>;


    [[nodiscard]]
    bool contains(
        PageId pageId,
        FrameId frameId
    ) const noexcept;


    ResidencyQueue residencyQueue_;

    Statistics statistics_;
};

} // namespace emmus::algorithms::replacement