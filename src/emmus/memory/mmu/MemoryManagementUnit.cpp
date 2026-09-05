#include "emmus/memory/mmu/MemoryManagementUnit.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace emmus::memory::mmu
{

MemoryManagementUnit::MemoryManagementUnit(
    PageTableType& pageTable,
    PhysicalMemoryManager& physicalMemory,
    PageReplacementPolicy& replacementPolicy,
    PageSize pageSize
) noexcept
    : pageTable_(pageTable)
    , physicalMemoryManager_(physicalMemory)
    , replacementPolicy_(replacementPolicy)
    , pageSize_(pageSize)
{
}

bool MemoryManagementUnit::registerPage(Page page)
{
    const PageId pageId = page.id();

    // A page registered with the MMU must begin in a nonresident state.
    if (page.isResident())
    {
        return false;
    }

    // Reject stale lower-level state for the same PageId. Registering a
    // page over an existing mapping would create an ambiguous ownership
    // relationship between the MMU's page registry and its lower layers.
    if (pageTable_.isMapped(pageId))
    {
        return false;
    }

    if (physicalMemoryManager_.isPageMapped(pageId))
    {
        return false;
    }

    const auto [iterator, inserted] =
        pages_.emplace(pageId, std::move(page));

    static_cast<void>(iterator);

    return inserted;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::access(const Access& access)
{
    const auto [pageId, pageOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_
        );

    Page* requestedPage = findPage(pageId);

    if (requestedPage == nullptr)
    {
        return failure(
            "Memory access references an unregistered page."
        );
    }

    // Process ownership is validated at the MMU boundary. The PageTable
    // itself is intentionally concerned only with PageId -> FrameId
    // mappings.
    if (requestedPage->processId() != access.processId())
    {
        return failure(
            "Memory access process does not own the requested page."
        );
    }

    PageTablePhysicalMemoryIntegration integration(
        pageTable_,
        physicalMemoryManager_
    );

    const auto mappedFrame = pageTable_.lookup(pageId);

    if (mappedFrame.has_value())
    {
        // A PageTable mapping must agree with both physical memory and
        // the Frame object before the access can be treated as resident.
        if (!integration.isMappingConsistent(pageId))
        {
            return failure(
                "Page-table and physical-memory mappings are inconsistent."
            );
        }

        if (!requestedPage->isResident())
        {
            return failure(
                "Page-table mapping exists for a nonresident page."
            );
        }

        if (!requestedPage->mappedFrame().has_value() ||
            requestedPage->mappedFrame().value() != mappedFrame.value())
        {
            return failure(
                "Registered page and page-table frame mappings are inconsistent."
            );
        }

        return processResidentAccess(
            access,
            pageId,
            mappedFrame.value()
        );
    }

    // A page with no PageTable mapping must also have no physical-memory
    // mapping. Otherwise the lower layers are inconsistent and the MMU
    // must not reinterpret that state as a normal page fault.
    if (physicalMemoryManager_.isPageMapped(pageId))
    {
        return failure(
            "Physical memory contains an unmapped page-table entry."
        );
    }

    if (requestedPage->isResident())
    {
        return failure(
            "Registered page is resident without a page-table mapping."
        );
    }

    // At this point the page is genuinely nonresident, so this is a
    // valid page fault.
    return processPageFault(access, pageId);
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::processResidentAccess(
    const Access& access,
    PageId pageId,
    FrameId frameId
)
{
    Page* requestedPage = findPage(pageId);

    if (requestedPage == nullptr)
    {
        return failure(
            "Resident page is not registered with the MMU."
        );
    }

    if (!requestedPage->isResident() ||
        !requestedPage->mappedFrame().has_value() ||
        requestedPage->mappedFrame().value() != frameId)
    {
        return failure(
            "Resident page state does not match the requested frame."
        );
    }

    PageTablePhysicalMemoryIntegration integration(
        pageTable_,
        physicalMemoryManager_
    );

    if (!integration.isMappingConsistent(pageId))
    {
        return failure(
            "Resident page mapping is inconsistent."
        );
    }

    // Complete the access first. This guarantees that an address
    // translation failure does not notify the replacement policy about
    // an access that did not actually complete.
    AccessResult result = completeAccess(
        access,
        *requestedPage,
        frameId,
        false,
        false,
        false
    );

    if (!result.success())
    {
        return result;
    }

    replacementPolicy_.pageAccessed(pageId, frameId);

    return result;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::processPageFault(
    const Access& access,
    PageId pageId
)
{
    Page* requestedPage = findPage(pageId);

    if (requestedPage == nullptr)
    {
        return failure(
            "Page-fault request references an unregistered page."
        );
    }

    if (requestedPage->isResident())
    {
        return failure(
            "Page-fault handler received a page that is already resident."
        );
    }

    PageTablePhysicalMemoryIntegration integration(
        pageTable_,
        physicalMemoryManager_
    );

    // The caller has already established that neither lower layer maps
    // the requested page. Recheck here because this method is also a
    // private invariant boundary.
    if (pageTable_.isMapped(pageId) ||
        physicalMemoryManager_.isPageMapped(pageId))
    {
        return failure(
            "Cannot handle page fault because the requested page already "
            "has lower-level mapping state."
        );
    }

    ++pageFaultCount_;

    /*
     * ------------------------------------------------------------------
     * Case 1: A free physical frame is available.
     * ------------------------------------------------------------------
     */
    if (physicalMemoryManager_.hasFreeFrame())
    {
        const auto frameId = integration.mapPage(pageId);

        if (!frameId.has_value())
        {
            return failure(
                "Page fault could not allocate a free physical frame."
            );
        }

        if (!physicalMemoryManager_.isValidFrameId(frameId.value()) ||
            !integration.isMappingConsistent(pageId))
        {
            // The integration layer should have rolled back its own
            // allocation on failure. If an unexpected invalid state is
            // observed after success, attempt to remove the mapping.
            static_cast<void>(integration.unmapPage(pageId));

            return failure(
                "Allocated frame does not produce a consistent page mapping."
            );
        }

        const auto pageMapResult =
            requestedPage->mapToFrame(frameId.value());

        if (pageMapResult.has_value())
        {
            static_cast<void>(integration.unmapPage(pageId));

            return failure(
                "Requested page could not be marked resident."
            );
        }

        AccessResult result = completeAccess(
            access,
            *requestedPage,
            frameId.value(),
            true,
            false,
            false
        );

        if (!result.success())
        {
            // Policy notification has not occurred yet, so the lower
            // mapping can be rolled back without requiring policy state
            // repair.
            static_cast<void>(requestedPage->unmapFromFrame());
            static_cast<void>(integration.unmapPage(pageId));

            return result;
        }

        replacementPolicy_.pageLoaded(pageId, frameId.value());

        return result;
    }

    /*
     * ------------------------------------------------------------------
     * Case 2: No free frame exists. Select a replacement victim.
     * ------------------------------------------------------------------
     */
    const auto victimFrame = replacementPolicy_.chooseVictim();

    if (!victimFrame.has_value())
    {
        return failure(
            "Page fault requires replacement, but the replacement policy "
            "did not provide a victim frame."
        );
    }

    if (!physicalMemoryManager_.isValidFrameId(victimFrame.value()))
    {
        return failure(
            "Replacement policy returned an invalid victim frame."
        );
    }

    const auto* selectedFrame =
        physicalMemoryManager_.frame(victimFrame.value());

    if (selectedFrame == nullptr ||
        !selectedFrame->isOccupied() ||
        !selectedFrame->mappedPage().has_value())
    {
        return failure(
            "Replacement policy selected an invalid or free victim frame."
        );
    }

    const PageId victimPageId =
        selectedFrame->mappedPage().value();

    Page* victimPage = findPage(victimPageId);

    if (victimPage == nullptr)
    {
        return failure(
            "Victim frame references a page that is not registered."
        );
    }

    if (!victimPage->isResident() ||
        !victimPage->mappedFrame().has_value() ||
        victimPage->mappedFrame().value() != victimFrame.value())
    {
        return failure(
            "Victim page state does not match the selected victim frame."
        );
    }

    if (!integration.isMappingConsistent(victimPageId))
    {
        return failure(
            "Victim page mapping is inconsistent."
        );
    }

    /*
     * Capture dirty state before Page::unmapFromFrame(), because unmapping
     * a Page intentionally clears both dirty and referenced state.
     */
    const bool dirtyEviction = victimPage->isDirty();

    /*
     * Validate the physical address before modifying any replacement
     * state. With no free frame available, the released victim frame is
     * the frame that must become available for the requested page.
     */
    const auto [requestedPageIdFromAddress, pageOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_
        );

    if (requestedPageIdFromAddress != pageId)
    {
        return failure(
            "Virtual-address decomposition does not match the requested page."
        );
    }

    try
    {
        static_cast<void>(
            emmus::memory::access::makePhysicalAddress(
                victimFrame.value(),
                pageOffset,
                pageSize_
            )
        );
    }
    catch (const std::overflow_error&)
    {
        return failure(
            "Physical-address calculation overflowed for the replacement frame."
        );
    }

    /*
     * Helper used when a replacement operation has already removed the
     * victim from the policy and lower-level mappings but cannot complete
     * the requested page load.
     *
     * There were no free frames before the victim was removed, so after
     * the failed requested-page mapping the victim frame is expected to
     * be the only available frame.
     */
    const auto restoreVictim =
        [&]() noexcept -> bool
    {
        const auto restoredFrame =
            integration.mapPage(victimPageId);

        if (!restoredFrame.has_value() ||
            restoredFrame.value() != victimFrame.value())
        {
            return false;
        }

        const auto mapResult =
            victimPage->mapToFrame(restoredFrame.value());

        if (mapResult.has_value())
        {
            static_cast<void>(
                integration.unmapPage(victimPageId)
            );

            return false;
        }

        if (!victimPage->isResident() ||
            !victimPage->mappedFrame().has_value() ||
            victimPage->mappedFrame().value() != victimFrame.value())
        {
            static_cast<void>(
                integration.unmapPage(victimPageId)
            );

            return false;
        }

        replacementPolicy_.pageLoaded(
            victimPageId,
            victimFrame.value()
        );

        return true;
    };

    /*
     * Evict the victim from the coordinated lower-level mapping.
     */
    if (!integration.releaseFrame(victimFrame.value()))
    {
        return failure(
            "Failed to release the selected victim frame."
        );
    }

    if (victimPage->unmapFromFrame().has_value())
    {
        // The lower-level mapping has already been released. Attempt to
        // restore it so the MMU does not knowingly leave the victim
        // detached from physical memory.
        const bool restored = restoreVictim();

        if (!restored)
        {
            return failure(
                "Victim frame was released, but the victim page could not "
                "be restored after its residency transition failed."
            );
        }

        return failure(
            "Victim page could not be marked nonresident."
        );
    }

    replacementPolicy_.pageRemoved(
        victimPageId,
        victimFrame.value()
    );

    /*
     * Load the requested page. Because the victim was the only frame
     * available immediately before release, mapPage() should return the
     * same frame. Verify that invariant explicitly.
     */
    const auto requestedFrame =
        integration.mapPage(pageId);

    if (!requestedFrame.has_value())
    {
        const bool restored = restoreVictim();

        if (!restored)
        {
            return failure(
                "Replacement failed and the victim mapping could not be restored."
            );
        }

        return failure(
            "Replacement failed because the requested page could not be mapped."
        );
    }

    if (requestedFrame.value() != victimFrame.value())
    {
        static_cast<void>(
            integration.unmapPage(pageId)
        );

        const bool restored = restoreVictim();

        if (!restored)
        {
            return failure(
                "Replacement selected an unexpected frame and the victim "
                "mapping could not be restored."
            );
        }

        return failure(
            "Replacement mapped the requested page to a frame different "
            "from the released victim frame."
        );
    }

    const auto pageMapResult =
        requestedPage->mapToFrame(requestedFrame.value());

    if (pageMapResult.has_value())
    {
        static_cast<void>(
            integration.unmapPage(pageId)
        );

        const bool restored = restoreVictim();

        if (!restored)
        {
            return failure(
                "Requested page could not become resident and the victim "
                "mapping could not be restored."
            );
        }

        return failure(
            "Requested page could not be marked resident after replacement."
        );
    }

    /*
     * The physical-address calculation was validated before eviction, so
     * this call should not encounter an overflow for the same frame,
     * offset, and page-size values. completeAccess still performs its own
     * defensive validation.
     */
    AccessResult result = completeAccess(
        access,
        *requestedPage,
        requestedFrame.value(),
        true,
        true,
        dirtyEviction
    );

    if (!result.success())
    {
        static_cast<void>(
            requestedPage->unmapFromFrame()
        );

        static_cast<void>(
            integration.unmapPage(pageId)
        );

        const bool restored = restoreVictim();

        if (!restored)
        {
            return failure(
                "Replacement access failed and the victim mapping could "
                "not be restored."
            );
        }

        return result;
    }

    /*
     * The replacement has now completed successfully. Only now should
     * the policy and MMU replacement statistics be updated.
     */
    replacementPolicy_.pageLoaded(
        pageId,
        requestedFrame.value()
    );

    ++pageReplacementCount_;

    if (dirtyEviction)
    {
        ++dirtyEvictionCount_;
    }

    return result;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::completeAccess(
    const Access& access,
    Page& page,
    FrameId frameId,
    bool pageFault,
    bool pageReplacement,
    bool dirtyEviction
)
{
    if (!page.isResident())
    {
        return failure(
            "Cannot complete an access for a nonresident page."
        );
    }

    if (!page.mappedFrame().has_value() ||
        page.mappedFrame().value() != frameId)
    {
        return failure(
            "Page residency does not match the supplied frame."
        );
    }

    const auto [pageId, pageOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_
        );

    if (page.id() != pageId)
    {
        return failure(
            "Access virtual address does not identify the supplied page."
        );
    }

    PhysicalAddress physicalAddress{0};

    try
    {
        physicalAddress =
            emmus::memory::access::makePhysicalAddress(
                frameId,
                pageOffset,
                pageSize_
            );
    }
    catch (const std::overflow_error&)
    {
        return failure(
            "Physical-address calculation overflowed."
        );
    }

    /*
     * Translation succeeded, so it is now safe to update the page's
     * reference/dirty state.
     */
    page.markReferenced();

    if (access.isWrite())
    {
        page.markDirty();
    }

    return AccessResult(
        true,
        pageFault,
        pageReplacement,
        std::optional<FrameId>{frameId},
        std::optional<PhysicalAddress>{physicalAddress},
        dirtyEviction
    );
}

MemoryManagementUnit::Page*
MemoryManagementUnit::findPage(PageId pageId) noexcept
{
    const auto iterator = pages_.find(pageId);

    if (iterator == pages_.end())
    {
        return nullptr;
    }

    return &iterator->second;
}

const MemoryManagementUnit::Page*
MemoryManagementUnit::findPage(PageId pageId) const noexcept
{
    const auto iterator = pages_.find(pageId);

    if (iterator == pages_.end())
    {
        return nullptr;
    }

    return &iterator->second;
}

void MemoryManagementUnit::reset()
{
    PageTablePhysicalMemoryIntegration integration(
        pageTable_,
        physicalMemoryManager_
    );

    /*
     * Only remove mappings that the integration layer confirms are
     * internally consistent. This avoids using the reset operation to
     * conceal an existing lower-level invariant violation.
     */
    for (auto& [pageId, page] : pages_)
    {
        if (!page.isResident())
        {
            continue;
        }

        if (!page.mappedFrame().has_value())
        {
            continue;
        }

        if (!integration.isMappingConsistent(pageId))
        {
            continue;
        }

        if (integration.unmapPage(pageId))
        {
            static_cast<void>(page.unmapFromFrame());
        }
    }

    replacementPolicy_.reset();

    pageFaultCount_ = 0;
    pageReplacementCount_ = 0;
    dirtyEvictionCount_ = 0;
}

MemoryManagementUnit::PageSize
MemoryManagementUnit::pageSize() const noexcept
{
    return pageSize_;
}

std::size_t
MemoryManagementUnit::registeredPageCount() const noexcept
{
    return pages_.size();
}

std::uint64_t
MemoryManagementUnit::pageFaultCount() const noexcept
{
    return pageFaultCount_;
}

std::uint64_t
MemoryManagementUnit::pageReplacementCount() const noexcept
{
    return pageReplacementCount_;
}

std::uint64_t
MemoryManagementUnit::dirtyEvictionCount() const noexcept
{
    return dirtyEvictionCount_;
}

MemoryManagementUnit::Page*
MemoryManagementUnit::page(PageId pageId) noexcept
{
    return findPage(pageId);
}

const MemoryManagementUnit::Page*
MemoryManagementUnit::page(PageId pageId) const noexcept
{
    return findPage(pageId);
}

MemoryManagementUnit::PageTableType&
MemoryManagementUnit::pageTable() noexcept
{
    return pageTable_;
}

const MemoryManagementUnit::PageTableType&
MemoryManagementUnit::pageTable() const noexcept
{
    return pageTable_;
}

MemoryManagementUnit::PhysicalMemoryManager&
MemoryManagementUnit::physicalMemoryManager() noexcept
{
    return physicalMemoryManager_;
}

const MemoryManagementUnit::PhysicalMemoryManager&
MemoryManagementUnit::physicalMemoryManager() const noexcept
{
    return physicalMemoryManager_;
}

MemoryManagementUnit::PageReplacementPolicy&
MemoryManagementUnit::replacementPolicy() noexcept
{
    return replacementPolicy_;
}

const MemoryManagementUnit::PageReplacementPolicy&
MemoryManagementUnit::replacementPolicy() const noexcept
{
    return replacementPolicy_;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::failure(std::string errorInformation)
{
    return AccessResult(
        false,
        false,
        false,
        std::nullopt,
        std::nullopt,
        false,
        std::move(errorInformation)
    );
}

} // namespace emmus::memory::mmu