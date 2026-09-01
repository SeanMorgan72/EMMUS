#pragma once

#include <optional>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::algorithms::replacement
{

/**
 * @brief Common interface for page-replacement policies.
 *
 * IPageReplacementPolicy defines the contract used by the pager to
 * communicate with a page-replacement policy without depending on
 * a specific replacement algorithm.
 *
 * Concrete implementations may provide FIFO, LRU, Clock, Optimal,
 * or future page-replacement algorithms.
 *
 * The policy is responsible for maintaining replacement-policy state
 * and selecting a victim frame. It does not perform physical-memory
 * management or page eviction.
 *
 * The PhysicalMemoryManager remains responsible for ownership and
 * management of Frame objects, including allocation and release.
 *
 * Ownership:
 * IPageReplacementPolicy does not own Page or Frame objects. A caller
 * that creates a concrete policy owns that policy instance.
 *
 * Lifetime:
 * A policy object must remain alive for as long as the pager may invoke
 * this interface. The caller must ensure that all interface calls have
 * ceased before the policy object is destroyed.
 *
 * Runtime polymorphism:
 * Concrete policies may be managed through a
 * std::unique_ptr<IPageReplacementPolicy>.
 */
class IPageReplacementPolicy
{
public:

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;


    /**
     * @brief Virtual destructor for polymorphic destruction.
     */
    virtual ~IPageReplacementPolicy() = default;


    IPageReplacementPolicy(
        const IPageReplacementPolicy&
    ) = delete;


    IPageReplacementPolicy& operator=(
        const IPageReplacementPolicy&
    ) = delete;


    IPageReplacementPolicy(
        IPageReplacementPolicy&&
    ) = delete;


    IPageReplacementPolicy& operator=(
        IPageReplacementPolicy&&
    ) = delete;


    /**
     * @brief Notifies the policy that a page has been loaded.
     *
     * The notification associates the supplied page with the physical
     * frame into which it was loaded.
     *
     * The policy must not perform the physical-memory allocation itself.
     *
     * @param pageId Identifier of the loaded page.
     * @param frameId Identifier of the frame containing the page.
     */
    virtual void pageLoaded(
        PageId pageId,
        FrameId frameId
    ) = 0;


    /**
     * @brief Notifies the policy that a resident page has been accessed.
     *
     * The notification allows a concrete policy to update whatever
     * replacement state is required by the access.
     *
     * @param pageId Identifier of the accessed page.
     * @param frameId Identifier of the frame containing the page.
     */
    virtual void pageAccessed(
        PageId pageId,
        FrameId frameId
    ) = 0;


    /**
     * @brief Notifies the policy that a page has been removed.
     *
     * The notification tells the policy that the page/frame association
     * is no longer resident and should no longer participate in victim
     * selection.
     *
     * The policy must not perform the physical-frame release itself.
     *
     * @param pageId Identifier of the removed page.
     * @param frameId Identifier of the released frame.
     */
    virtual void pageRemoved(
        PageId pageId,
        FrameId frameId
    ) = 0;


    /**
     * @brief Selects a victim frame for page replacement.
     *
     * The policy determines which currently tracked frame should be
     * considered for replacement.
     *
     * The policy only selects the victim. Actual eviction, page-table
     * modification, write-back, and frame release remain responsibilities
     * of the memory-management subsystem.
     *
     * @return The selected victim FrameId.
     *
     * @return std::nullopt when no eligible victim exists.
     */
    [[nodiscard]]
    virtual std::optional<FrameId> chooseVictim() = 0;


    /**
     * @brief Resets all state maintained by the policy.
     *
     * After reset(), the policy behaves as though it has not received
     * any page-loaded, page-accessed, or page-removed notifications.
     */
    virtual void reset() = 0;


protected:

    /**
     * @brief Constructs the policy interface.
     */
    IPageReplacementPolicy() = default;
};

} // namespace emmus::algorithms::replacement