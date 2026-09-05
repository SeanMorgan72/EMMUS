#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessResult.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/mmu/IMemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual/Page.hpp"

namespace emmus::memory::mmu
{

/**
 * @brief Enhanced Memory Management Unit implementation.
 *
 * The MemoryManagementUnit coordinates:
 * - virtual-address decomposition,
 * - process/page validation,
 * - page-table lookup,
 * - page-fault detection,
 * - free-frame allocation,
 * - page replacement,
 * - dirty-page eviction tracking,
 * - page residency state,
 * - replacement-policy notifications, and
 * - virtual-to-physical address translation.
 *
 * The MMU does not own the PageTable or PhysicalMemoryManager. Both are
 * supplied by reference and must outlive this object.
 */
class MemoryManagementUnit final : public IMemoryManagementUnit
{
public:
    using Access = emmus::memory::access::MemoryAccess;
    using AccessResult = emmus::memory::access::MemoryAccessResult;
    using AccessOperation =
        emmus::memory::access::MemoryAccessOperation;

    using PageSize = emmus::memory::access::PageSize;
    using VirtualAddress = emmus::memory::access::VirtualAddress;
    using PhysicalAddress = emmus::memory::access::PhysicalAddress;

    using ProcessId = emmus::memory::identifiers::ProcessId;
    using PageId = emmus::memory::identifiers::PageId;
    using FrameId = emmus::memory::identifiers::FrameId;

    using Page = emmus::memory::virtual_memory::Page;

    using PageTableType = emmus::memory::mmu::PageTable;
    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;

    using PageReplacementPolicy =
        emmus::algorithms::replacement::IPageReplacementPolicy;

    /**
     * @brief Constructs an MMU.
     *
     * @param pageTable Page-table owned externally.
     * @param physicalMemory Physical-memory manager owned externally.
     * @param replacementPolicy Replacement-policy instance owned externally.
     * @param pageSize Configured virtual-page/frame size.
     */
    MemoryManagementUnit(
        PageTableType& pageTable,
        PhysicalMemoryManager& physicalMemory,
        PageReplacementPolicy& replacementPolicy,
        PageSize pageSize
    ) noexcept;

    ~MemoryManagementUnit() override = default;

    MemoryManagementUnit(const MemoryManagementUnit&) = delete;
    MemoryManagementUnit& operator=(const MemoryManagementUnit&) = delete;

    MemoryManagementUnit(MemoryManagementUnit&&) = delete;
    MemoryManagementUnit& operator=(MemoryManagementUnit&&) = delete;

    /**
     * @brief Registers a virtual page with the MMU.
     *
     * A page must be nonresident and must not already have lower-level
     * PageTable or physical-memory state associated with its PageId.
     *
     * @return true if registration succeeds; false otherwise.
     */
    bool registerPage(Page page) override;

    /**
     * @brief Processes one memory access.
     *
     * The access may complete directly if the page is resident or may
     * trigger page-fault handling and, when necessary, page replacement.
     */
    AccessResult access(const Access& access) override;

    /**
     * @brief Resets MMU state and replacement-policy state.
     *
     * Resident pages are unmapped from the coordinated page-table and
     * physical-memory state before counters and policy state are reset.
     */
    void reset() override;

    /**
     * @brief Returns the configured page size.
     */
    PageSize pageSize() const noexcept;

    /**
     * @brief Returns the number of pages registered with the MMU.
     */
    std::size_t registeredPageCount() const noexcept;

    /**
     * @brief Returns the number of page faults detected by the MMU.
     */
    std::uint64_t pageFaultCount() const noexcept;

    /**
     * @brief Returns the number of completed page replacements.
     */
    std::uint64_t pageReplacementCount() const noexcept;

    /**
     * @brief Returns the number of completed dirty-page evictions.
     */
    std::uint64_t dirtyEvictionCount() const noexcept;

    /**
     * @brief Returns a mutable registered page, or nullptr if absent.
     */
    Page* page(PageId pageId) noexcept;

    /**
     * @brief Returns a read-only registered page, or nullptr if absent.
     */
    const Page* page(PageId pageId) const noexcept;

    /**
     * @brief Returns the underlying page table.
     */
    PageTableType& pageTable() noexcept;

    /**
     * @brief Returns the underlying page table read-only.
     */
    const PageTableType& pageTable() const noexcept;

    /**
     * @brief Returns the underlying physical-memory manager.
     */
    PhysicalMemoryManager& physicalMemoryManager() noexcept;

    /**
     * @brief Returns the underlying physical-memory manager read-only.
     */
    const PhysicalMemoryManager& physicalMemoryManager() const noexcept;

    /**
     * @brief Returns the configured replacement policy.
     */
    PageReplacementPolicy& replacementPolicy() noexcept;

    /**
     * @brief Returns the configured replacement policy read-only.
     */
    const PageReplacementPolicy& replacementPolicy() const noexcept;

private:
    /**
     * @brief Completes a successful access to a resident page.
     *
     * Physical-address calculation is performed before page state is
     * modified so a translation overflow cannot leave partially updated
     * page state.
     */
    AccessResult processResidentAccess(
        const Access& access,
        PageId pageId,
        FrameId frameId
    );

    /**
     * @brief Handles a page fault for the requested page.
     *
     * The method uses a free frame when available. Otherwise it selects
     * and evicts a victim according to the configured replacement policy.
     */
    AccessResult processPageFault(
        const Access& access,
        PageId pageId
    );

    /**
     * @brief Completes the access after a page is mapped to a frame.
     *
     * This validates the page/frame relationship, calculates the physical
     * address, updates referenced/dirty state, and constructs the result.
     */
    AccessResult completeAccess(
        const Access& access,
        Page& page,
        FrameId frameId,
        bool pageFault,
        bool pageReplacement,
        bool dirtyEviction
    );

    /**
     * @brief Finds a registered page.
     */
    Page* findPage(PageId pageId) noexcept;

    /**
     * @brief Finds a registered page read-only.
     */
    const Page* findPage(PageId pageId) const noexcept;

    /**
     * @brief Constructs a failed memory-access result.
     */
    static AccessResult failure(std::string errorInformation);

    PageTableType& pageTable_;
    PhysicalMemoryManager& physicalMemoryManager_;
    PageReplacementPolicy& replacementPolicy_;

    PageSize pageSize_;

    std::unordered_map<PageId, Page> pages_;

    std::uint64_t pageFaultCount_{0};
    std::uint64_t pageReplacementCount_{0};
    std::uint64_t dirtyEvictionCount_{0};
};

} // namespace emmus::memory::mmu