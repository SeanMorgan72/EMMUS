#pragma once

#include <optional>

#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"

namespace emmus::memory::mmu
{

/**
 * @brief Coordinates PageTable mappings with physical-frame allocation.
 *
 * PageTable remains responsible for virtual-page-to-physical-frame mappings.
 * PhysicalMemoryManager remains responsible for physical-frame allocation,
 * occupancy, and release.
 *
 * This class coordinates operations that must update both components while
 * preserving their independent responsibilities.
 *
 * Callers requiring an integrated page mapping, unmapping, or frame release
 * should use this class rather than modifying the PageTable and
 * PhysicalMemoryManager independently.
 */
class PageTablePhysicalMemoryIntegration
{
public:

    using PageId = PageTable::PageId;
    using FrameId = PageTable::FrameId;


    /**
     * @brief Creates an integration coordinator over existing components.
     *
     * PageTable and PhysicalMemoryManager remain owned by the caller.
     *
     * @param pageTable Page table to coordinate.
     * @param physicalMemory Physical-memory manager to coordinate.
     */
    PageTablePhysicalMemoryIntegration(
        PageTable& pageTable,
        physical::PhysicalMemoryManager& physicalMemory
    ) noexcept;


    PageTablePhysicalMemoryIntegration(
        const PageTablePhysicalMemoryIntegration&
    ) = delete;

    PageTablePhysicalMemoryIntegration& operator=(
        const PageTablePhysicalMemoryIntegration&
    ) = delete;

    PageTablePhysicalMemoryIntegration(
        PageTablePhysicalMemoryIntegration&&
    ) = delete;

    PageTablePhysicalMemoryIntegration& operator=(
        PageTablePhysicalMemoryIntegration&&
    ) = delete;

    ~PageTablePhysicalMemoryIntegration() = default;


    /**
     * @brief Allocates a physical frame and creates the page-table mapping.
     *
     * The physical frame is allocated first. A PageTable entry is created
     * only after physical allocation succeeds.
     *
     * If physical allocation fails, neither component is modified.
     *
     * If PageTable::map() unexpectedly fails after physical allocation,
     * the newly allocated physical frame is released.
     *
     * @param pageId Virtual page to map.
     *
     * @return The allocated FrameId on success.
     *
     * @return std::nullopt when the page is already mapped, physical
     *         allocation fails, or the page-table mapping cannot be created.
     */
    [[nodiscard]]
    std::optional<FrameId> mapPage(
        PageId pageId
    ) noexcept;


    /**
     * @brief Removes a page mapping and releases its physical frame.
     *
     * Before modifying either component, the operation verifies that the
     * PageTable and PhysicalMemoryManager agree about the PageId/FrameId
     * relationship.
     *
     * A stale or conflicting relationship is rejected without modifying
     * either component.
     *
     * @param pageId Virtual page to unmap.
     *
     * @return true when the mapping and physical frame were released.
     *
     * @return false when the page is unmapped, inconsistent, or release fails.
     */
    [[nodiscard]]
    bool unmapPage(
        PageId pageId
    ) noexcept;


    /**
     * @brief Releases a physical frame through the integration boundary.
     *
     * The page-table mapping associated with the frame is removed as part
     * of the operation.
     *
     * A stale or conflicting page-table relationship prevents the release.
     *
     * @param frameId Physical frame to release.
     *
     * @return true when both the page-table mapping and physical frame
     *         were released.
     *
     * @return false when the frame is invalid, free, or inconsistent.
     */
    [[nodiscard]]
    bool releaseFrame(
        FrameId frameId
    ) noexcept;


    /**
     * @brief Returns the physical frame for a consistently mapped page.
     *
     * @param pageId Virtual page to query.
     *
     * @return The FrameId when both components agree.
     *
     * @return std::nullopt when the page is unmapped or inconsistent.
     */
    [[nodiscard]]
    std::optional<FrameId> frameForPage(
        PageId pageId
    ) const noexcept;


    /**
     * @brief Determines whether a page has a consistent mapping.
     *
     * A consistent mapping means:
     *
     * PageTable:
     *     PageId -> FrameId
     *
     * PhysicalMemoryManager:
     *     PageId -> same FrameId
     *
     * Frame:
     *     FrameId -> same PageId
     */
    [[nodiscard]]
    bool isMappingConsistent(
        PageId pageId
    ) const noexcept;


    /**
     * @brief Verifies consistency of all active mappings.
     *
     * This is a verification operation only. It does not repair inconsistent
     * state that was created by bypassing this integration boundary.
     *
     * @return true when the page table and physical memory are consistent.
     */
    [[nodiscard]]
    bool isConsistent() const noexcept;


private:

    PageTable& pageTable_;

    physical::PhysicalMemoryManager& physicalMemory_;
};

} // namespace emmus::memory::mmu