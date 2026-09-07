#include "emmus/memory/mmu/MemoryManagementUnit.hpp"

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
    : pageTable_(pageTable),
      physicalMemoryManager_(physicalMemory),
      replacementPolicy_(replacementPolicy),
      pageSize_(pageSize)
{
}

bool MemoryManagementUnit::registerPage(Page page)
{
    if (page.isResident())
    {
        return false;
    }

    const PageId pageId = page.id();

    if (pages_.contains(pageId))
    {
        return false;
    }

    if (pageTable_.lookup(pageId).has_value())
    {
        return false;
    }

    if (physicalMemoryManager_.isPageMapped(pageId))
    {
        return false;
    }

    pages_.emplace(pageId, std::move(page));

    return true;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::access(const Access& access)
{
    const auto [pageId, pageOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_);

    (void)pageOffset;

    Page* requestedPage = findPage(pageId);

    if (requestedPage == nullptr)
    {
        return failure(
            "Memory access references an unregistered virtual page");
    }

    if (requestedPage->processId() != access.processId())
    {
        return failure(
            "Memory access process does not own the requested virtual page");
    }

    PageTablePhysicalMemoryIntegration integration{
        pageTable_,
        physicalMemoryManager_
    };

    const auto mappedFrame =
        pageTable_.lookup(pageId);

    if (mappedFrame.has_value())
    {
        if (!integration.isMappingConsistent(pageId))
        {
            return failure(
                "Page-table and physical-memory mapping is inconsistent");
        }

        if (!requestedPage->isResident())
        {
            return failure(
                "Page-table mapping exists for a nonresident page");
        }

        if (!requestedPage->mappedFrame().has_value())
        {
            return failure(
                "Resident page does not have an associated frame");
        }

        if (requestedPage->mappedFrame().value() != mappedFrame.value())
        {
            return failure(
                "Page frame does not match the page-table mapping");
        }

        return processResidentAccess(
            access,
            pageId,
            mappedFrame.value());
    }

    if (physicalMemoryManager_.isPageMapped(pageId))
    {
        return failure(
            "Physical-memory mapping exists without a page-table mapping");
    }

    if (requestedPage->isResident())
    {
        return failure(
            "Page is marked resident without a page-table mapping");
    }

    return processPageFault(
        access,
        pageId);
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::processResidentAccess(
    const Access& access,
    PageId pageId,
    FrameId frameId)
{
    Page* page = findPage(pageId);

    if (page == nullptr)
    {
        return failure(
            "Registered page could not be found");
    }

    if (!page->isResident())
    {
        return failure(
            "Resident access requested for a nonresident page");
    }

    if (!page->mappedFrame().has_value())
    {
        return failure(
            "Resident page does not have an associated frame");
    }

    if (page->mappedFrame().value() != frameId)
    {
        return failure(
            "Resident page frame does not match the requested frame");
    }

    PageTablePhysicalMemoryIntegration integration{
        pageTable_,
        physicalMemoryManager_
    };

    if (!integration.isMappingConsistent(pageId))
    {
        return failure(
            "Resident page mapping is inconsistent");
    }

    const auto result =
        completeAccess(
            access,
            *page,
            frameId,
            false,
            false,
            false);

    if (!result.success())
    {
        return result;
    }

    replacementPolicy_.pageAccessed(
        pageId,
        frameId);

    return result;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::processPageFault(
    const Access& access,
    PageId pageId)
{
    Page* requestedPage = findPage(pageId);

    if (requestedPage == nullptr)
    {
        return failure(
            "Page-fault handling could not find the requested page");
    }

    if (requestedPage->isResident())
    {
        return failure(
            "Page-fault handling requested for an already resident page");
    }

    PageTablePhysicalMemoryIntegration integration{
        pageTable_,
        physicalMemoryManager_
    };

    if (pageTable_.lookup(pageId).has_value())
    {
        return failure(
            "Page-fault handling found an existing page-table mapping");
    }

    if (physicalMemoryManager_.isPageMapped(pageId))
    {
        return failure(
            "Page-fault handling found an existing physical-memory mapping");
    }

    /*
     * The MMU has detected a page fault at this point.
     *
     * The existing test contract expects pageFaultCount() to increase
     * even when the subsequent frame-allocation operation fails.
     */
    ++pageFaultCount_;

    /*
     * Prefer a free frame whenever one is available. The replacement
     * policy must not be invoked in this case.
     */
    if (physicalMemoryManager_.hasFreeFrame())
    {
        const auto frameId =
            integration.mapPage(pageId);

        if (!frameId.has_value())
        {
            return failure(
                "A free physical frame was available but could not be mapped");
        }

        const FrameId selectedFrame = frameId.value();

        if (!physicalMemoryManager_.isValidFrameId(selectedFrame))
        {
            (void)integration.releaseFrame(selectedFrame);

            return failure(
                "Physical-memory manager returned an invalid frame identifier");
        }

        if (!integration.isMappingConsistent(pageId))
        {
            (void)integration.releaseFrame(selectedFrame);

            return failure(
                "New page mapping is inconsistent");
        }

        auto framePage =
            physicalMemoryManager_.frame(selectedFrame);

        if (framePage == nullptr ||
            !framePage->isOccupied() ||
            !framePage->mappedPage().has_value() ||
            framePage->mappedPage().value() != pageId)
        {
            (void)integration.releaseFrame(selectedFrame);

            return failure(
                "Allocated frame does not contain the requested page");
        }

        const auto mapResult =
            requestedPage->mapToFrame(selectedFrame);

        if (mapResult.has_value())
        {
            (void)integration.releaseFrame(selectedFrame);

            return failure(
                "Requested page could not be mapped to the allocated frame");
        }

        if (!integration.isMappingConsistent(pageId))
        {
            (void)requestedPage->unmapFromFrame();
            (void)integration.releaseFrame(selectedFrame);

            return failure(
                "Page mapping became inconsistent after residency update");
        }

        const auto result =
            completeAccess(
                access,
                *requestedPage,
                selectedFrame,
                true,
                false,
                false);

        if (!result.success())
        {
            (void)requestedPage->unmapFromFrame();
            (void)integration.releaseFrame(selectedFrame);

            return result;
        }

        replacementPolicy_.pageLoaded(
            pageId,
            selectedFrame);

        replacementPolicy_.pageAccessed(
            pageId,
            selectedFrame);

        return result;
    }

    /*
    * Physical memory is full. The replacement policy must select the
    * victim frame.
    */
    const auto victimFrameOptional =
        replacementPolicy_.chooseVictim();

    if (!victimFrameOptional.has_value())
    {
        return pageFaultFailure(
            "Physical memory is full and the replacement policy could not select a victim");
    }

    const FrameId victimFrame = victimFrameOptional.value();

    if (!physicalMemoryManager_.isValidFrameId(victimFrame))
    {
        return failure(
            "Replacement policy returned an invalid victim frame");
    }

    const auto* victimPhysicalFrame =
        physicalMemoryManager_.frame(victimFrame);

    if (victimPhysicalFrame == nullptr ||
        !victimPhysicalFrame->isOccupied() ||
        !victimPhysicalFrame->mappedPage().has_value())
    {
        return failure(
            "Replacement policy selected a free or invalid victim frame");
    }

    const PageId victimPageId =
        victimPhysicalFrame->mappedPage().value();

    Page* victimPage =
        findPage(victimPageId);

    if (victimPage == nullptr)
    {
        return failure(
            "Victim frame references an unregistered page");
    }

    if (!victimPage->isResident())
    {
        return failure(
            "Victim page is not resident");
    }

    if (!victimPage->mappedFrame().has_value())
    {
        return failure(
            "Victim page does not have an associated frame");
    }

    if (victimPage->mappedFrame().value() != victimFrame)
    {
        return failure(
            "Victim page frame does not match the selected victim frame");
    }

    if (!integration.isMappingConsistent(victimPageId))
    {
        return failure(
            "Victim page mapping is inconsistent");
    }

    const bool victimWasDirty =
        victimPage->isDirty();

    const bool victimWasReferenced =
        victimPage->isReferenced();

    /*
     * Validate the requested virtual address and physical translation
     * before modifying the existing victim state. This prevents a
     * translation failure from destroying a valid resident mapping.
     */
    const auto [requestedPageId, requestedOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_);

    if (requestedPageId != pageId)
    {
        return failure(
            "Virtual-address decomposition does not match the requested page");
    }

    try
    {
        (void)emmus::memory::access::makePhysicalAddress(
            victimFrame,
            requestedOffset,
            pageSize_);
    }
    catch (const std::overflow_error& exception)
    {
        return failure(exception.what());
    }

    /*
     * Dirty eviction represents the simulated write-back of the victim
     * page before its frame is reused.
     */
    if (victimWasDirty)
    {
        victimPage->clearDirty();
    }

    /*
     * Restore the victim if any part of the replacement transaction fails.
     */
    const auto restoreVictim =
        [&]() -> bool
        {
            if (victimPage->isResident())
            {
                return false;
            }

            if (!integration.mapPage(victimPageId).has_value())
            {
                return false;
            }

            const auto restoredFrame =
                integration.frameForPage(victimPageId);

            if (!restoredFrame.has_value() ||
                restoredFrame.value() != victimFrame)
            {
                (void)integration.releaseFrame(victimFrame);
                return false;
            }

            const auto restoreResult =
                victimPage->mapToFrame(victimFrame);

            if (restoreResult.has_value())
            {
                (void)integration.releaseFrame(victimFrame);
                return false;
            }

            if (victimWasDirty)
            {
                victimPage->markDirty();
            }

            if (victimWasReferenced)
            {
                victimPage->markReferenced();
            }

            replacementPolicy_.pageLoaded(
                victimPageId,
                victimFrame);

            return integration.isMappingConsistent(
                victimPageId);
        };

    /*
     * Remove the victim from the coordinated page-table and physical
     * memory state.
     */
    if (!integration.releaseFrame(victimFrame))
    {
        if (victimWasDirty)
        {
            victimPage->markDirty();
        }

        return failure(
            "Victim frame could not be released");
    }

    if (victimPage->isResident())
    {
        const auto unmapResult =
            victimPage->unmapFromFrame();

        if (unmapResult.has_value())
        {
            /*
             * The physical frame has already been released. Attempt to
             * restore the victim mapping before reporting failure.
             */
            (void)restoreVictim();

            return failure(
                "Victim page could not be unmapped");
        }
    }

    replacementPolicy_.pageRemoved(
        victimPageId,
        victimFrame);

    /*
     * Reuse the released frame for the requested page.
     */
    const auto requestedFrame =
        integration.mapPage(pageId);

    if (!requestedFrame.has_value() ||
        requestedFrame.value() != victimFrame)
    {
        /*
         * The frame should have been reused. If it was not, attempt to
         * restore the victim.
         */
        if (requestedFrame.has_value())
        {
            (void)integration.releaseFrame(requestedFrame.value());
        }

        (void)restoreVictim();

        return failure(
            "Requested page could not be mapped into the victim frame");
    }

    const auto requestedMapResult =
        requestedPage->mapToFrame(victimFrame);

    if (requestedMapResult.has_value())
    {
        (void)integration.releaseFrame(victimFrame);
        (void)restoreVictim();

        return failure(
            "Requested page could not be made resident");
    }

    if (!integration.isMappingConsistent(pageId))
    {
        (void)requestedPage->unmapFromFrame();
        (void)integration.releaseFrame(victimFrame);
        (void)restoreVictim();

        return failure(
            "Requested page mapping is inconsistent after replacement");
    }

    const auto result =
        completeAccess(
            access,
            *requestedPage,
            victimFrame,
            true,
            true,
            victimWasDirty);

    if (!result.success())
    {
        (void)requestedPage->unmapFromFrame();
        (void)integration.releaseFrame(victimFrame);

        (void)restoreVictim();

        return result;
    }

    replacementPolicy_.pageLoaded(
        pageId,
        victimFrame);

    replacementPolicy_.pageAccessed(
        pageId,
        victimFrame);

    ++pageReplacementCount_;

    if (victimWasDirty)
    {
        ++dirtyEvictionCount_;

        replacementPolicy_.recordDirtyEviction();
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
    bool dirtyEviction)
{
    if (!page.isResident())
    {
        return failure(
            "Cannot complete an access for a nonresident page");
    }

    if (!page.mappedFrame().has_value())
    {
        return failure(
            "Cannot complete an access for a page without a frame");
    }

    if (page.mappedFrame().value() != frameId)
    {
        return failure(
            "Page frame does not match the requested frame");
    }

    const auto [pageId, pageOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_);

    if (pageId != page.id())
    {
        return failure(
            "Virtual-address decomposition does not match the resident page");
    }

    PhysicalAddress physicalAddress{0};

    try
    {
        physicalAddress =
            emmus::memory::access::makePhysicalAddress(
                frameId,
                pageOffset,
                pageSize_);
    }
    catch (const std::overflow_error& exception)
    {
        return failure(exception.what());
    }

    /*
     * Only modify page state after physical-address calculation has
     * succeeded.
     */
    page.markReferenced();

    if (access.isWrite())
    {
        page.markDirty();
    }

    return AccessResult{
        true,
        pageFault,
        pageReplacement,
        frameId,
        physicalAddress,
        dirtyEviction
    };
}

MemoryManagementUnit::Page*
MemoryManagementUnit::findPage(PageId pageId) noexcept
{
    const auto iterator =
        pages_.find(pageId);

    if (iterator == pages_.end())
    {
        return nullptr;
    }

    return &iterator->second;
}

const MemoryManagementUnit::Page*
MemoryManagementUnit::findPage(PageId pageId) const noexcept
{
    const auto iterator =
        pages_.find(pageId);

    if (iterator == pages_.end())
    {
        return nullptr;
    }

    return &iterator->second;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::failure(std::string errorInformation)
{
    return AccessResult{
        false,
        false,
        false,
        std::nullopt,
        std::nullopt,
        false,
        std::move(errorInformation)
    };
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::pageFaultFailure(std::string errorInformation)
{
    return AccessResult{
        false,
        true,
        false,
        std::nullopt,
        std::nullopt,
        false,
        std::move(errorInformation)
    };
}

void MemoryManagementUnit::reset()
{
    PageTablePhysicalMemoryIntegration integration{
        pageTable_,
        physicalMemoryManager_
    };

    /*
     * Clear every successfully resident page. The integration layer is
     * used so that page-table and physical-memory state remain coordinated.
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

        if (!integration.releaseFrame(page.mappedFrame().value()))
        {
            continue;
        }

        (void)page.unmapFromFrame();
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

} // namespace emmus::memory::mmu