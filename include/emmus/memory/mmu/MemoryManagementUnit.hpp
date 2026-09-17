#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessResult.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/mmu/IMemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual_memory/Page.hpp"
#include "emmus/statistics/PageFaultStatistics.hpp"
#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"

namespace emmus::memory::mmu
{

class MemoryManagementUnit final
    : public IMemoryManagementUnit
{
public:
    using Access =
        emmus::memory::access::MemoryAccess;

    using AccessResult =
        emmus::memory::access::MemoryAccessResult;

    using PageSize =
        emmus::memory::access::PageSize;

    using VirtualAddress =
        emmus::memory::access::VirtualAddress;

    using ProcessId =
        emmus::memory::identifiers::ProcessId;

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;

    using Page =
        emmus::memory::virtual_memory::Page;

    using PageFaultStatistics =
        emmus::statistics::PageFaultStatistics;

    using ReplacementPolicy =
        emmus::algorithms::replacement::IPageReplacementPolicy;

    using VirtualPageNumber =
        std::uint64_t;

    MemoryManagementUnit(
        PageTable& pageTable,
        emmus::memory::physical::PhysicalMemoryManager&
            physicalMemoryManager,
        ReplacementPolicy& replacementPolicy,
        PageSize pageSize);

    ~MemoryManagementUnit() override = default;

    MemoryManagementUnit(
        const MemoryManagementUnit&) = delete;

    MemoryManagementUnit& operator=(
        const MemoryManagementUnit&) = delete;

    MemoryManagementUnit(
        MemoryManagementUnit&&) = delete;

    MemoryManagementUnit& operator=(
        MemoryManagementUnit&&) = delete;

    bool registerPage(Page page) override;

    bool registerPage(
        Page page,
        VirtualPageNumber virtualPageNumber);

    AccessResult access(const Access& access) override;

    void reset() override;

    [[nodiscard]]
    std::size_t registeredPageCount() const noexcept;

    [[nodiscard]]
    std::size_t pageFaultCount() const noexcept;

    [[nodiscard]]
    std::size_t pageReplacementCount() const noexcept;

    [[nodiscard]]
    std::size_t dirtyEvictionCount() const noexcept;

    [[nodiscard]]
    std::size_t pageSize() const noexcept;

    [[nodiscard]]
    Page* page(PageId pageId) noexcept;

    [[nodiscard]]
    const Page* page(PageId pageId) const noexcept;

    [[nodiscard]]
    const PageFaultStatistics&
    pageFaultStatistics() const noexcept;

private:
    [[nodiscard]]
    std::optional<PageId> resolvePageId(
        ProcessId processId,
        VirtualPageNumber virtualPageNumber) const noexcept;

    AccessResult processResidentAccess(
        const Access& access,
        PageId pageId,
        FrameId frameId);

    AccessResult processPageFault(
        const Access& access,
        PageId pageId);

    AccessResult completeAccess(
        const Access& access,
        Page& page,
        FrameId frameId,
        bool pageFault,
        bool pageReplacement,
        bool dirtyEviction);

    [[nodiscard]]
    Page* findPage(PageId pageId) noexcept;

    [[nodiscard]]
    const Page* findPage(PageId pageId) const noexcept;

    [[nodiscard]]
    AccessResult failure(
        const std::string& errorInformation,
        bool pageFault = false) const;

    PageTable& pageTable_;

    emmus::memory::physical::PhysicalMemoryManager&
        physicalMemoryManager_;

    ReplacementPolicy& replacementPolicy_;

    PageSize pageSize_;

    std::unordered_map<PageId, Page> pages_;

    std::unordered_map<
        ProcessId,
        std::unordered_map<
            VirtualPageNumber,
            PageId>> virtualPageMappings_;

    PageFaultStatistics pageFaultStatistics_;

    std::size_t pageFaultCount_{0U};

    std::size_t pageReplacementCount_{0U};

    std::size_t dirtyEvictionCount_{0U};
};

} // namespace emmus::memory::mmu