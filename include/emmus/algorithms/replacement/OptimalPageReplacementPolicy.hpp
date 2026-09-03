#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/statistics/PageReplacementStatistics.hpp"

namespace emmus::algorithms::replacement
{

/**
 * @brief Optimal page-replacement policy.
 *
 * Optimal replacement selects the resident page whose next required
 * use occurs farthest in the future. A page that has no future use
 * is preferred over any page that will be referenced again.
 *
 * The policy requires knowledge of the complete future reference
 * sequence. The sequence is supplied independently from the common
 * replacement-policy interface so that FIFO, LRU, and Clock do not
 * require knowledge of future workload state.
 *
 * The policy tracks resident page/frame associations and evaluates
 * each resident page against the remaining reference sequence when
 * chooseVictim() is called.
 *
 * When multiple candidates have the same future-use distance,
 * residency order is used as the deterministic tie-breaker.
 *
 * The policy does not own or modify Page or Frame objects, perform
 * physical-memory management, update page tables, or perform the
 * actual eviction.
 */
class OptimalPageReplacementPolicy final
    : public IPageReplacementPolicy
{
public:

    using Statistics =
        emmus::statistics::PageReplacementStatistics;

    using ReferenceSequence =
        std::vector<PageId>;


    /**
     * @brief Constructs an empty Optimal replacement policy.
     */
    OptimalPageReplacementPolicy() = default;


    OptimalPageReplacementPolicy(
        const OptimalPageReplacementPolicy&
    ) = delete;


    OptimalPageReplacementPolicy& operator=(
        const OptimalPageReplacementPolicy&
    ) = delete;


    OptimalPageReplacementPolicy(
        OptimalPageReplacementPolicy&&
    ) noexcept = default;


    OptimalPageReplacementPolicy& operator=(
        OptimalPageReplacementPolicy&&
    ) noexcept = default;


    ~OptimalPageReplacementPolicy() override = default;


    /**
     * @brief Supplies the complete reference sequence.
     *
     * The reference sequence represents the ordered future memory
     * accesses available to the Optimal policy.
     *
     * Supplying a new sequence resets the policy's current reference
     * position to the beginning of that sequence.
     *
     * @param referenceSequence Ordered page-reference sequence.
     */
    void setReferenceSequence(
        ReferenceSequence referenceSequence
    );


    /**
     * @brief Returns the configured reference sequence.
     */
    [[nodiscard]]
    const ReferenceSequence& referenceSequence() const noexcept;


    /**
     * @brief Records a newly loaded page/frame association.
     *
     * New mappings are appended in residency order.
     *
     * Duplicate page/frame associations are ignored.
     * Conflicting page or frame mappings are also ignored.
     *
     * @param pageId Page that became resident.
     * @param frameId Frame containing the page.
     */
    void pageLoaded(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Records a memory access.
     *
     * For Optimal replacement, the access notification also advances
     * the current position in the configured reference sequence.
     *
     * This advancement occurs even when the page is not currently
     * resident because Optimal must remain synchronized with the
     * complete workload sequence.
     *
     * A resident page/frame association is otherwise validated before
     * any residency-specific state is modified.
     *
     * @param pageId Accessed page.
     * @param frameId Frame containing the page when resident.
     */
    void pageAccessed(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Removes a page/frame association from Optimal tracking.
     *
     * Invalid or mismatched notifications are ignored.
     *
     * @param pageId Page being removed.
     * @param frameId Frame previously containing the page.
     */
    void pageRemoved(
        PageId pageId,
        FrameId frameId
    ) override;


    /**
     * @brief Selects the optimal replacement victim.
     *
     * The resident page whose next reference occurs farthest in the
     * future is selected.
     *
     * A resident page with no future reference is preferred over every
     * page that will be referenced again.
     *
     * When candidates have equivalent future-use distances, residency
     * order provides deterministic behavior.
     *
     * The selected victim remains tracked until pageRemoved() is called.
     *
     * @return The selected victim FrameId.
     * @return std::nullopt when no resident frame is tracked.
     */
    [[nodiscard]]
    std::optional<FrameId> chooseVictim() override;


    /**
     * @brief Resets residency, reference-sequence position, and statistics.
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
        std::size_t residencyOrder;
    };


    using ResidencyCollection =
        std::vector<ResidencyEntry>;


    /**
     * @brief Finds an exact page/frame association.
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
     * @brief Finds the next occurrence of a page in the future sequence.
     *
     * @param pageId Page whose future use is being queried.
     *
     * @return Index of the next occurrence, or std::nullopt if the
     *         page is not referenced again.
     */
    [[nodiscard]]
    std::optional<std::size_t> nextUse(
        PageId pageId
    ) const noexcept;


    /**
     * @brief Advances the reference-sequence position for an access.
     *
     * The next matching occurrence is consumed. If no matching
     * occurrence remains, the sequence position is left unchanged.
     */
    void advanceReferencePosition(
        PageId pageId
    ) noexcept;


    ResidencyCollection residencyCollection_;

    ReferenceSequence referenceSequence_;

    std::size_t referencePosition_{0U};

    std::size_t nextResidencyOrder_{0U};

    Statistics statistics_;
};

} // namespace emmus::algorithms::replacement