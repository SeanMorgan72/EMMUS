#include "emmus/memory/mmu/MemoryManagementUnit.hpp"

#include <utility>

#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"

namespace emmus::memory::mmu
{

MemoryManagementUnit::MemoryManagementUnit(
    PageTable& pageTable,
    emmus::memory::physical::PhysicalMemoryManager& physicalMemoryManager,
    ReplacementPolicy& replacementPolicy,
    PageSize pageSize)
    : pageTable_(pageTable),
      physicalMemoryManager_(physicalMemoryManager),
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
    const ProcessId processId = page.processId();

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

    virtualPageMappings_[processId].emplace(
        pageId.value(),
        pageId);

    return true;
}

bool MemoryManagementUnit::registerPage(
    Page page,
    VirtualPageNumber virtualPageNumber)
{
    if (page.isResident())
    {
        return false;
    }

    const PageId pageId = page.id();
    const ProcessId processId = page.processId();

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

    const auto [pageIterator, pageInserted] =
        pages_.emplace(
            pageId,
            std::move(page));

    if (!pageInserted)
    {
        return false;
    }

    auto [processMappingIterator, processMappingInserted] =
        virtualPageMappings_.try_emplace(processId);

    auto& processMappings =
        processMappingIterator->second;

    const auto [mappingIterator, mappingInserted] =
        processMappings.emplace(
            virtualPageNumber,
            pageId);

    if (!mappingInserted)
    {
        pages_.erase(pageIterator);

        if (processMappings.empty())
        {
            virtualPageMappings_.erase(
                processMappingIterator);
        }

        return false;
    }

    return true;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::access(const Access& access)
{
    const auto [virtualPageId, pageOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_);

    const auto resolvedPageId =
        resolvePageId(
            access.processId(),
            virtualPageId.value());

    if (!resolvedPageId.has_value())
    {
        return failure(
            "Memory access references an unregistered virtual page");
    }

    const PageId pageId =
        resolvedPageId.value();

    Page* requestedPage =
        findPage(pageId);

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

    (void)pageOffset;

    PageTablePhysicalMemoryIntegration integration{
        pageTable_,
        physicalMemoryManager_};

    pageFaultStatistics_.recordAccess(
        access.processId());

    const auto mappedFrame =
        pageTable_.lookup(pageId);

    if (mappedFrame.has_value())
    {
        if (!integration.isMappingConsistent(pageId))
        {
            return failure(
                "Page-table and physical-memory mapping is inconsistent");
        }

        return processResidentAccess(
            access,
            pageId,
            mappedFrame.value());
    }

    return processPageFault(
        access,
        pageId);
}

void MemoryManagementUnit::reset()
{
    PageTablePhysicalMemoryIntegration integration{
        pageTable_,
        physicalMemoryManager_};

    for (auto& [pageId, page] : pages_)
    {
        if (!page.isResident())
        {
            continue;
        }

        const auto mappedFrame =
            page.mappedFrame();

        if (!mappedFrame.has_value())
        {
            continue;
        }

        if (!integration.isMappingConsistent(pageId))
        {
            continue;
        }

        if (!integration.releaseFrame(
                mappedFrame.value()))
        {
            continue;
        }

        (void)page.unmapFromFrame();
    }

    replacementPolicy_.reset();

    pageFaultStatistics_.reset();
    pageFaultCount_ = 0U;
    pageReplacementCount_ = 0U;
    dirtyEvictionCount_ = 0U;
}

std::size_t
MemoryManagementUnit::registeredPageCount() const noexcept
{
    return pages_.size();
}

std::size_t
MemoryManagementUnit::pageFaultCount() const noexcept
{
    return pageFaultCount_;
}

std::size_t
MemoryManagementUnit::pageReplacementCount() const noexcept
{
    return pageReplacementCount_;
}

std::size_t
MemoryManagementUnit::dirtyEvictionCount() const noexcept
{
    return dirtyEvictionCount_;
}

std::size_t MemoryManagementUnit::pageSize() const noexcept
{
    return pageSize_.value();
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

const MemoryManagementUnit::PageFaultStatistics&
MemoryManagementUnit::pageFaultStatistics() const noexcept
{
    return pageFaultStatistics_;
}

std::optional<MemoryManagementUnit::PageId>
MemoryManagementUnit::resolvePageId(
    ProcessId processId,
    VirtualPageNumber virtualPageNumber) const noexcept
{
    const auto processIterator =
        virtualPageMappings_.find(processId);

    if (processIterator ==
        virtualPageMappings_.end())
    {
        return std::nullopt;
    }

    const auto& processMappings =
        processIterator->second;

    const auto mappingIterator =
        processMappings.find(virtualPageNumber);

    if (mappingIterator ==
        processMappings.end())
    {
        return std::nullopt;
    }

    return mappingIterator->second;
}

MemoryManagementUnit::AccessResult
MemoryManagementUnit::processResidentAccess(
    const Access& access,
    PageId pageId,
    FrameId frameId)
{
    Page* page =
        findPage(pageId);

    if (page == nullptr)
    {
        return failure(
            "Page-table mapping references an unregistered page");
    }

    if (!page->isResident())
    {
        return failure(
            "Page-table mapping is inconsistent with page residency");
    }

    if (!page->mappedFrame().has_value() ||
        page->mappedFrame().value() != frameId)
    {
        return failure(
            "Page-table mapping is inconsistent with page frame");
    }

    return completeAccess(
        access,
        *page,
        frameId,
        false,
        false,
        false);
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

    ++pageFaultCount_;
    pageFaultStatistics_.recordPageFault(
        access.processId());

    /*
     * Prefer a free frame whenever one is available.
     * The replacement policy must not be invoked in this case.
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

        return result;
    }

    /*
     * Physical memory is full.
     * The replacement policy must select the victim frame.
     */
    const auto victimFrameOptional =
        replacementPolicy_.chooseVictim();

    if (!victimFrameOptional.has_value())
    {
        return AccessResult{
            false,
            true,
            false,
            std::nullopt,
            std::nullopt,
            false,
            "Physical memory is full and the replacement policy could not select a victim"
        };
    }

    const FrameId victimFrame =
        victimFrameOptional.value();

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

    const auto [requestedPageId, requestedOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_);


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

    if (victimWasDirty)
    {
        victimPage->clearDirty();
    }

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
            (void)restoreVictim();

            return failure(
                "Victim page could not be unmapped");
        }
    }

    replacementPolicy_.pageRemoved(
        victimPageId,
        victimFrame);

    const auto requestedFrame =
        integration.mapPage(pageId);

    if (!requestedFrame.has_value() ||
        requestedFrame.value() != victimFrame)
    {
        if (requestedFrame.has_value())
        {
            (void)integration.releaseFrame(
                requestedFrame.value());
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

    const auto [virtualPageId, pageOffset] =
        emmus::memory::access::decomposeVirtualAddress(
            access.virtualAddress(),
            pageSize_);

    const auto resolvedPageId =
        resolvePageId(
            page.processId(),
            virtualPageId.value());

    if (!resolvedPageId.has_value() ||
        resolvedPageId.value() != page.id())
    {
        return failure(
            "Memory access virtual page does not match registered page");
    }

    const auto physicalAddress =
        emmus::memory::access::PhysicalAddress{
            frameId.value() * pageSize_.value() +
            pageOffset.value()};

    page.markReferenced();

    if (access.operation() ==
        emmus::memory::access::MemoryAccessOperation::Write)
    {
        page.markDirty();
    }

    replacementPolicy_.pageAccessed(
        page.id(),
        frameId);

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
MemoryManagementUnit::failure(
    const std::string& errorInformation,
    bool pageFault) const
{
    return AccessResult{
        false,
        pageFault,
        false,
        std::nullopt,
        std::nullopt,
        false,
        errorInformation
    };
}

} // namespace emmus::memory::mmu