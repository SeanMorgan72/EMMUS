#include <cstdint>
#include <memory>
#include <optional>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/ClockPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/LRUPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/OptimalPageReplacementPolicy.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessResult.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/mmu/MemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual/Page.hpp"

namespace emmus::tests
{

class MemoryManagementUnitIntegrationTest : public ::testing::Test
{
protected:
    using FIFOPageReplacementPolicy =
        emmus::algorithms::replacement::FIFOPageReplacementPolicy;

    using MemoryManagementUnit =
        emmus::memory::mmu::MemoryManagementUnit;

    using Page =
        emmus::memory::virtual_memory::Page;

    using PageTable =
        emmus::memory::mmu::PageTable;

    using PageTablePhysicalMemoryIntegration =
        emmus::memory::mmu::PageTablePhysicalMemoryIntegration;

    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;

    using MemoryAccess =
        emmus::memory::access::MemoryAccess;

    using MemoryAccessOperation =
        emmus::memory::access::MemoryAccessOperation;

    using VirtualAddress =
        emmus::memory::access::VirtualAddress;

    using PageSize =
        emmus::memory::access::PageSize;

    using AccessSequenceNumber =
        emmus::memory::access::AccessSequenceNumber;

    using ProcessId =
        emmus::memory::identifiers::ProcessId;

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;

    static constexpr std::uint64_t kPageSize = 4096;

    static constexpr ProcessId kProcess1{100};
    static constexpr ProcessId kProcess2{200};

    static constexpr PageId kPage0{0};
    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};

    static constexpr FrameId kFrame0{0};
    static constexpr FrameId kFrame1{1};

    void SetUp() override
    {
        pageSize = PageSize{kPageSize};

        mmu = std::make_unique<MemoryManagementUnit>(
            pageTable,
            physicalMemory,
            replacementPolicy,
            pageSize
        );
    }

    MemoryAccess read(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0
    ) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) * kPageSize
                    + offset
            },
            MemoryAccessOperation::Read,
            AccessSequenceNumber{sequence}
        );
    }

    MemoryAccess write(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0
    ) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) * kPageSize
                    + offset
            },
            MemoryAccessOperation::Write,
            AccessSequenceNumber{sequence}
        );
    }

    void registerPage(
        PageId pageId,
        ProcessId processId = kProcess1
    )
    {
        ASSERT_TRUE(
            mmu->registerPage(
                Page{pageId, processId}
            )
        );
    }

    void expectIntegratedStateIsConsistent() const
    {
        EXPECT_TRUE(
            integration.isConsistent()
        );
    }

    PageTable pageTable;
    PhysicalMemoryManager physicalMemory{2};
    FIFOPageReplacementPolicy replacementPolicy;
    PageSize pageSize{kPageSize};

    PageTablePhysicalMemoryIntegration integration{
        pageTable,
        physicalMemory
    };

    std::unique_ptr<MemoryManagementUnit> mmu;
};


// ============================================================================
// Initial State
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    FirstAccessLoadsPageIntoPhysicalMemoryAndCreatesPageTableMapping
)
{
    registerPage(kPage0);

    const auto result = mmu->access(read(kProcess1, kPage0, 123, 1));

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), kFrame0);

    ASSERT_TRUE(result.physicalAddress().has_value());
    EXPECT_EQ(
        result.physicalAddress().value(),
        emmus::memory::access::PhysicalAddress{123}
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.capacity(),
        2U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        1U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        1U
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    const Page* page = mmu->page(kPage0);

    ASSERT_NE(page, nullptr);

    EXPECT_TRUE(page->isResident());
    EXPECT_TRUE(page->isReferenced());
    EXPECT_FALSE(page->isDirty());

    ASSERT_TRUE(page->mappedFrame().has_value());
    EXPECT_EQ(
        page->mappedFrame().value(),
        kFrame0
    );

    expectIntegratedStateIsConsistent();
}


TEST_F(
    MemoryManagementUnitIntegrationTest,
    PhysicalAddressTranslationIncludesFrameBaseAndPageOffset
)
{
    registerPage(kPage0);
    registerPage(kPage1);

    const auto first = mmu->access(
        read(kProcess1, kPage0, 0, 1)
    );

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(first.pageFault());
    ASSERT_FALSE(first.pageReplacement());

    EXPECT_EQ(
        first.frameId(),
        std::optional<FrameId>{kFrame0}
    );

    const std::uint64_t offset = 123;

    const auto second = mmu->access(
        read(kProcess1, kPage1, offset, 2)
    );

    ASSERT_TRUE(second.success());
    EXPECT_TRUE(second.pageFault());
    EXPECT_FALSE(second.pageReplacement());
    EXPECT_FALSE(second.dirtyEviction());

    EXPECT_EQ(
        second.frameId(),
        std::optional<FrameId>{kFrame1}
    );

    ASSERT_TRUE(second.physicalAddress().has_value());

    EXPECT_EQ(
        second.physicalAddress().value(),
        emmus::memory::access::PhysicalAddress{
            kPageSize + offset
        }
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        2U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// US-802: Free Frame Allocation
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    FreeFrameIsAllocatedBeforeFIFOReplacement
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    const auto first = mmu->access(read(kProcess1, kPage0, 64, 1));

    ASSERT_TRUE(first.success());
    EXPECT_TRUE(first.pageFault());
    EXPECT_FALSE(first.pageReplacement());
    EXPECT_FALSE(first.dirtyEviction());

    ASSERT_TRUE(first.frameId().has_value());
    EXPECT_EQ(
        first.frameId().value(),
        kFrame0
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        1U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        1U
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    expectIntegratedStateIsConsistent();

    const auto second = mmu->access(read(kProcess1, kPage1, 128, 2));

    ASSERT_TRUE(second.success());
    EXPECT_TRUE(second.pageFault());
    EXPECT_FALSE(second.pageReplacement());
    EXPECT_FALSE(second.dirtyEviction());

    ASSERT_TRUE(second.frameId().has_value());
    EXPECT_EQ(
        second.frameId().value(),
        kFrame1
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        2U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    expectIntegratedStateIsConsistent();

    const auto third = mmu->access(read(kProcess1, kPage2, 256, 3));

    ASSERT_TRUE(third.success());
    EXPECT_TRUE(third.pageFault());
    EXPECT_TRUE(third.pageReplacement());
    EXPECT_FALSE(third.dirtyEviction());

    ASSERT_TRUE(third.frameId().has_value());
    EXPECT_EQ(
        third.frameId().value(),
        kFrame0
    );

    ASSERT_TRUE(third.physicalAddress().has_value());
    EXPECT_EQ(
        third.physicalAddress().value(),
        emmus::memory::access::PhysicalAddress{
            256
        }
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        1U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_FALSE(
        pageTable.lookup(kPage0).has_value()
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage2),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_FALSE(
        physicalMemory.frameForPage(kPage0).has_value()
    );

    const Page* evictedPage = mmu->page(kPage0);

    ASSERT_NE(evictedPage, nullptr);

    EXPECT_FALSE(evictedPage->isResident());
    EXPECT_FALSE(evictedPage->isDirty());
    EXPECT_FALSE(evictedPage->isReferenced());

    const Page* requestedPage = mmu->page(kPage2);

    ASSERT_NE(requestedPage, nullptr);

    EXPECT_TRUE(requestedPage->isResident());
    EXPECT_FALSE(requestedPage->isDirty());
    EXPECT_TRUE(requestedPage->isReferenced());

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// Dirty Page Replacement
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    DirtyFIFOReplacementReportsDirtyEviction
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    const auto first = mmu->access(
        write(kProcess1, kPage0, 0, 1)
    );

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(first.pageFault());
    ASSERT_FALSE(first.pageReplacement());

    const auto second = mmu->access(
        read(kProcess1, kPage1, 0, 2)
    );

    ASSERT_TRUE(second.success());
    ASSERT_TRUE(second.pageFault());
    ASSERT_FALSE(second.pageReplacement());

    const Page* dirtyPage = mmu->page(kPage0);

    ASSERT_NE(dirtyPage, nullptr);
    EXPECT_TRUE(dirtyPage->isResident());
    EXPECT_TRUE(dirtyPage->isDirty());

    const auto third = mmu->access(
        read(kProcess1, kPage2, 0, 3)
    );

    ASSERT_TRUE(third.success());

    EXPECT_TRUE(third.pageFault());
    EXPECT_TRUE(third.pageReplacement());
    EXPECT_TRUE(third.dirtyEviction());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().dirtyEvictionCount(),
        1U
    );

    const Page* evictedPage = mmu->page(kPage0);

    ASSERT_NE(evictedPage, nullptr);

    EXPECT_FALSE(evictedPage->isResident());
    EXPECT_FALSE(evictedPage->isDirty());
    EXPECT_FALSE(evictedPage->isReferenced());

    EXPECT_FALSE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::nullopt
    );

    const Page* requestedPage = mmu->page(kPage2);

    ASSERT_NE(requestedPage, nullptr);

    EXPECT_TRUE(requestedPage->isResident());
    EXPECT_FALSE(requestedPage->isDirty());
    EXPECT_TRUE(requestedPage->isReferenced());

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// US-802: Replacement Must Not Occur While a Free Frame Exists
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    ReplacementOccursOnlyAfterAllFreeFramesHaveBeenConsumed
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    const auto first = mmu->access(
        read(kProcess1, kPage0, 0, 1)
    );

    ASSERT_TRUE(first.success());
    EXPECT_TRUE(first.pageFault());
    EXPECT_FALSE(first.pageReplacement());

    EXPECT_EQ(
        first.frameId(),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        1U
    );

    const auto second = mmu->access(
        read(kProcess1, kPage1, 0, 2)
    );

    ASSERT_TRUE(second.success());
    EXPECT_TRUE(second.pageFault());
    EXPECT_FALSE(second.pageReplacement());

    EXPECT_EQ(
        second.frameId(),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    const auto third = mmu->access(
        read(kProcess1, kPage2, 0, 3)
    );

    ASSERT_TRUE(third.success());
    EXPECT_TRUE(third.pageFault());
    EXPECT_TRUE(third.pageReplacement());
    EXPECT_FALSE(third.dirtyEviction());

    EXPECT_EQ(
        third.frameId(),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        1U
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        std::optional<FrameId>{kFrame0}
    );

    expectIntegratedStateIsConsistent();
}


TEST_F(
    MemoryManagementUnitIntegrationTest,
    MultipleFreeFramesAreConsumedDeterministicallyBeforeReplacement
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    ASSERT_EQ(
        physicalMemory.freeFrameCount(),
        2U
    );

    const auto first = mmu->access(
        read(kProcess1, kPage0, 0, 1)
    );

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(first.pageFault());
    EXPECT_FALSE(first.pageReplacement());

    ASSERT_EQ(
        first.frameId(),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    const auto second = mmu->access(
        read(kProcess1, kPage1, 0, 2)
    );

    ASSERT_TRUE(second.success());
    ASSERT_TRUE(second.pageFault());
    EXPECT_FALSE(second.pageReplacement());

    ASSERT_EQ(
        second.frameId(),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        2U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    expectIntegratedStateIsConsistent();
}


TEST_F(
    MemoryManagementUnitIntegrationTest,
    FreeFrameAllocationDoesNotIncrementReplacementStatistics
)
{
    registerPage(kPage0);
    registerPage(kPage1);

    const auto first = mmu->access(
        read(kProcess1, kPage0, 0, 1)
    );

    ASSERT_TRUE(first.success());

    const auto second = mmu->access(
        read(kProcess1, kPage1, 0, 2)
    );

    ASSERT_TRUE(second.success());

    EXPECT_TRUE(first.pageFault());
    EXPECT_TRUE(second.pageFault());

    EXPECT_FALSE(first.pageReplacement());
    EXPECT_FALSE(second.pageReplacement());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        2U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().dirtyEvictionCount(),
        0U
    );

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// Process Isolation
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    ProcessIsolationPreventsAccessToAnotherProcessesPage
)
{
    registerPage(
        kPage0,
        kProcess1
    );

    const auto result = mmu->access(
        read(kProcess2, kPage0, 0, 1)
    );

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        0U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        2U
    );

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// Consistency
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    PageTableAndPhysicalMemoryRemainConsistentAfterReplacement
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 1)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage1, 0, 2)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage2, 0, 3)
        ).success()
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );

    EXPECT_EQ(
        pageTable.size(),
        physicalMemory.allocatedFrameCount()
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    const Page* page0 = mmu->page(kPage0);
    const Page* page1 = mmu->page(kPage1);
    const Page* page2 = mmu->page(kPage2);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page1, nullptr);
    ASSERT_NE(page2, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_TRUE(page1->isResident());
    EXPECT_TRUE(page2->isResident());
}


// ============================================================================
// US-803: LRU Replacement Algorithm Invocation
// ============================================================================

class MemoryManagementUnitLRUIntegrationTest : public ::testing::Test
{
protected:
    using LRUPageReplacementPolicy =
        emmus::algorithms::replacement::LRUPageReplacementPolicy;

    using MemoryManagementUnit =
        emmus::memory::mmu::MemoryManagementUnit;

    using Page =
        emmus::memory::virtual_memory::Page;

    using PageTable =
        emmus::memory::mmu::PageTable;

    using PageTablePhysicalMemoryIntegration =
        emmus::memory::mmu::PageTablePhysicalMemoryIntegration;

    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;

    using MemoryAccess =
        emmus::memory::access::MemoryAccess;

    using MemoryAccessOperation =
        emmus::memory::access::MemoryAccessOperation;

    using VirtualAddress =
        emmus::memory::access::VirtualAddress;

    using PageSize =
        emmus::memory::access::PageSize;

    using AccessSequenceNumber =
        emmus::memory::access::AccessSequenceNumber;

    using ProcessId =
        emmus::memory::identifiers::ProcessId;

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;

    static constexpr std::uint64_t kPageSize = 4096;

    static constexpr ProcessId kProcess1{100};

    static constexpr PageId kPage0{0};
    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};

    static constexpr FrameId kFrame0{0};
    static constexpr FrameId kFrame1{1};

    PageTable pageTable;
    PhysicalMemoryManager physicalMemory{2};
    LRUPageReplacementPolicy replacementPolicy;
    PageSize pageSize{kPageSize};

    PageTablePhysicalMemoryIntegration integration{
        pageTable,
        physicalMemory
    };

    std::unique_ptr<MemoryManagementUnit> mmu;

    void SetUp() override
    {
        mmu = std::make_unique<MemoryManagementUnit>(
            pageTable,
            physicalMemory,
            replacementPolicy,
            pageSize
        );
    }

    MemoryAccess read(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0
    ) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) * kPageSize
                    + offset
            },
            MemoryAccessOperation::Read,
            AccessSequenceNumber{sequence}
        );
    }

    void registerPage(
        PageId pageId,
        ProcessId processId = kProcess1
    )
    {
        ASSERT_TRUE(
            mmu->registerPage(
                Page{pageId, processId}
            )
        );
    }

    void expectIntegratedStateIsConsistent() const
    {
        EXPECT_TRUE(
            integration.isConsistent()
        );
    }
};


TEST_F(
    MemoryManagementUnitLRUIntegrationTest,
    LRUChoosesLeastRecentlyUsedResidentPage
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 1)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage1, 0, 2)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 3)
        ).success()
    );

    const auto result = mmu->access(
        read(kProcess1, kPage2, 0, 4)
    );

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());

    EXPECT_EQ(
        result.frameId(),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_FALSE(
        pageTable.lookup(kPage1).has_value()
    );

    const Page* evictedPage = mmu->page(kPage1);

    ASSERT_NE(evictedPage, nullptr);

    EXPECT_FALSE(evictedPage->isResident());
    EXPECT_FALSE(evictedPage->isDirty());
    EXPECT_FALSE(evictedPage->isReferenced());

    const Page* page0 = mmu->page(kPage0);
    const Page* page2 = mmu->page(kPage2);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page2, nullptr);

    EXPECT_TRUE(page0->isResident());
    EXPECT_TRUE(page2->isResident());

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// US-804: Clock Replacement Algorithm Invocation
// ============================================================================

class MemoryManagementUnitClockIntegrationTest : public ::testing::Test
{
protected:
    using ClockPageReplacementPolicy =
        emmus::algorithms::replacement::ClockPageReplacementPolicy;

    using MemoryManagementUnit =
        emmus::memory::mmu::MemoryManagementUnit;

    using Page =
        emmus::memory::virtual_memory::Page;

    using PageTable =
        emmus::memory::mmu::PageTable;

    using PageTablePhysicalMemoryIntegration =
        emmus::memory::mmu::PageTablePhysicalMemoryIntegration;

    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;

    using MemoryAccess =
        emmus::memory::access::MemoryAccess;

    using MemoryAccessOperation =
        emmus::memory::access::MemoryAccessOperation;

    using VirtualAddress =
        emmus::memory::access::VirtualAddress;

    using PageSize =
        emmus::memory::access::PageSize;

    using AccessSequenceNumber =
        emmus::memory::access::AccessSequenceNumber;

    using ProcessId =
        emmus::memory::identifiers::ProcessId;

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;

    static constexpr std::uint64_t kPageSize = 4096;

    static constexpr ProcessId kProcess1{100};

    static constexpr PageId kPage0{0};
    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};

    static constexpr FrameId kFrame0{0};
    static constexpr FrameId kFrame1{1};

    PageTable pageTable;
    PhysicalMemoryManager physicalMemory{2};
    ClockPageReplacementPolicy replacementPolicy;
    PageSize pageSize{kPageSize};

    PageTablePhysicalMemoryIntegration integration{
        pageTable,
        physicalMemory
    };

    std::unique_ptr<MemoryManagementUnit> mmu;

    void SetUp() override
    {
        mmu = std::make_unique<MemoryManagementUnit>(
            pageTable,
            physicalMemory,
            replacementPolicy,
            pageSize
        );
    }

    MemoryAccess read(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0
    ) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) * kPageSize
                    + offset
            },
            MemoryAccessOperation::Read,
            AccessSequenceNumber{sequence}
        );
    }

    void registerPage(
        PageId pageId,
        ProcessId processId = kProcess1
    )
    {
        ASSERT_TRUE(
            mmu->registerPage(
                Page{pageId, processId}
            )
        );
    }

    void expectIntegratedStateIsConsistent() const
    {
        EXPECT_TRUE(
            integration.isConsistent()
        );
    }
};


TEST_F(
    MemoryManagementUnitClockIntegrationTest,
    ClockClearsReferencedBitBeforeSelectingVictim
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 1)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage1, 0, 2)
        ).success()
    );

    const auto result = mmu->access(
        read(kProcess1, kPage2, 0, 3)
    );

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());

    EXPECT_EQ(
        result.frameId(),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_FALSE(
        pageTable.lookup(kPage0).has_value()
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        std::optional<FrameId>{kFrame0}
    );

    const Page* evictedPage = mmu->page(kPage0);

    ASSERT_NE(evictedPage, nullptr);

    EXPECT_FALSE(evictedPage->isResident());
    EXPECT_FALSE(evictedPage->isReferenced());

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// US-805: Optimal Replacement Algorithm Invocation
// ============================================================================

class MemoryManagementUnitOptimalIntegrationTest : public ::testing::Test
{
protected:
    using OptimalPageReplacementPolicy =
        emmus::algorithms::replacement::OptimalPageReplacementPolicy;

    using MemoryManagementUnit =
        emmus::memory::mmu::MemoryManagementUnit;

    using Page =
        emmus::memory::virtual_memory::Page;

    using PageTable =
        emmus::memory::mmu::PageTable;

    using PageTablePhysicalMemoryIntegration =
        emmus::memory::mmu::PageTablePhysicalMemoryIntegration;

    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;

    using MemoryAccess =
        emmus::memory::access::MemoryAccess;

    using MemoryAccessOperation =
        emmus::memory::access::MemoryAccessOperation;

    using VirtualAddress =
        emmus::memory::access::VirtualAddress;

    using PageSize =
        emmus::memory::access::PageSize;

    using AccessSequenceNumber =
        emmus::memory::access::AccessSequenceNumber;

    using ProcessId =
        emmus::memory::identifiers::ProcessId;

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;

    static constexpr std::uint64_t kPageSize = 4096;

    static constexpr ProcessId kProcess1{100};

    static constexpr PageId kPage0{0};
    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};

    static constexpr FrameId kFrame0{0};
    static constexpr FrameId kFrame1{1};

    PageTable pageTable;
    PhysicalMemoryManager physicalMemory{2};
    OptimalPageReplacementPolicy replacementPolicy;
    PageSize pageSize{kPageSize};

    PageTablePhysicalMemoryIntegration integration{
        pageTable,
        physicalMemory
    };

    std::unique_ptr<MemoryManagementUnit> mmu;

    void SetUp() override
    {
        mmu = std::make_unique<MemoryManagementUnit>(
            pageTable,
            physicalMemory,
            replacementPolicy,
            pageSize
        );
    }

    MemoryAccess read(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0
    ) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) * kPageSize
                    + offset
            },
            MemoryAccessOperation::Read,
            AccessSequenceNumber{sequence}
        );
    }

    MemoryAccess write(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0
    ) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) * kPageSize
                    + offset
            },
            MemoryAccessOperation::Write,
            AccessSequenceNumber{sequence}
        );
    }

    void registerPage(
        PageId pageId,
        ProcessId processId = kProcess1
    )
    {
        ASSERT_TRUE(
            mmu->registerPage(
                Page{pageId, processId}
            )
        );
    }

    void expectIntegratedStateIsConsistent() const
    {
        EXPECT_TRUE(
            integration.isConsistent()
        );
    }
};


TEST_F(
    MemoryManagementUnitOptimalIntegrationTest,
    OptimalChoosesPageWithFarthestNextUse
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    replacementPolicy.setReferenceSequence(
        {
            kPage0,
            kPage1,
            kPage2,
            kPage0,
            kPage1
        }
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 1)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage1, 0, 2)
        ).success()
    );

    /*
     * Future sequence:
     *
     *   page 0 -> sequence 5
     *   page 1 -> sequence 4
     *
     * Therefore page 0 is used farther in the future and page 1
     * should be selected as the victim.
     */
    const auto result = mmu->access(
        read(kProcess1, kPage2, 0, 3)
    );

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());

    EXPECT_EQ(
        result.frameId(),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_FALSE(
        pageTable.lookup(kPage1).has_value()
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        std::optional<FrameId>{kFrame1}
    );

    const Page* evictedPage = mmu->page(kPage1);

    ASSERT_NE(evictedPage, nullptr);

    EXPECT_FALSE(evictedPage->isResident());
    EXPECT_FALSE(evictedPage->isDirty());
    EXPECT_FALSE(evictedPage->isReferenced());

    const Page* page0 = mmu->page(kPage0);
    const Page* page2 = mmu->page(kPage2);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page2, nullptr);

    EXPECT_TRUE(page0->isResident());
    EXPECT_TRUE(page2->isResident());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        1U
    );

    expectIntegratedStateIsConsistent();
}


TEST_F(
    MemoryManagementUnitOptimalIntegrationTest,
    FaultingAccessAdvancesOptimalReferenceSequence
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    replacementPolicy.setReferenceSequence(
        {
            kPage0,
            kPage1,
            kPage0,
            kPage2,
            kPage0
        }
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 1)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage1, 0, 2)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 3)
        ).success()
    );

    const auto result = mmu->access(
        read(kProcess1, kPage2, 0, 4)
    );

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());

    EXPECT_EQ(
        result.frameId(),
        std::optional<FrameId>{kFrame1}
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_FALSE(
        pageTable.lookup(kPage1).has_value()
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        std::optional<FrameId>{kFrame1}
    );

    expectIntegratedStateIsConsistent();
}


TEST_F(
    MemoryManagementUnitOptimalIntegrationTest,
    RepeatedOptimalReplacementsMaintainConsistentMappings
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 1)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage1, 0, 2)
        ).success()
    );

    const auto third = mmu->access(
        read(kProcess1, kPage2, 0, 3)
    );

    ASSERT_TRUE(third.success());
    EXPECT_TRUE(third.pageFault());
    EXPECT_TRUE(third.pageReplacement());

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    expectIntegratedStateIsConsistent();

    const auto fourth = mmu->access(
        read(kProcess1, kPage1, 0, 4)
    );

    ASSERT_TRUE(fourth.success());
    EXPECT_FALSE(fourth.pageFault());
    EXPECT_FALSE(fourth.pageReplacement());

    expectIntegratedStateIsConsistent();

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );
}


TEST_F(
    MemoryManagementUnitOptimalIntegrationTest,
    DirtyOptimalReplacementReportsDirtyEviction
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    const auto first = mmu->access(
        write(kProcess1, kPage0, 0, 1)
    );

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(first.pageFault());
    ASSERT_FALSE(first.pageReplacement());

    const auto second = mmu->access(
        read(kProcess1, kPage1, 0, 2)
    );

    ASSERT_TRUE(second.success());
    ASSERT_TRUE(second.pageFault());
    ASSERT_FALSE(second.pageReplacement());

    const auto third = mmu->access(
        read(kProcess1, kPage2, 0, 3)
    );

    ASSERT_TRUE(third.success());
    EXPECT_TRUE(third.pageFault());
    EXPECT_TRUE(third.pageReplacement());
    EXPECT_TRUE(third.dirtyEviction());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().dirtyEvictionCount(),
        1U
    );

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// US-802: Zero Physical Frames
// ============================================================================

TEST(
    MemoryManagementUnitZeroFrameIntegrationTest,
    PageFaultFailsWithoutCreatingInvalidMapping
)
{
    using MemoryManagementUnit =
        emmus::memory::mmu::MemoryManagementUnit;

    using Page =
        emmus::memory::virtual_memory::Page;

    using PageTable =
        emmus::memory::mmu::PageTable;

    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;

    using FIFOPageReplacementPolicy =
        emmus::algorithms::replacement::FIFOPageReplacementPolicy;

    using MemoryAccess =
        emmus::memory::access::MemoryAccess;

    using MemoryAccessOperation =
        emmus::memory::access::MemoryAccessOperation;

    using VirtualAddress =
        emmus::memory::access::VirtualAddress;

    using PageSize =
        emmus::memory::access::PageSize;

    using AccessSequenceNumber =
        emmus::memory::access::AccessSequenceNumber;

    using ProcessId =
        emmus::memory::identifiers::ProcessId;

    using PageId =
        emmus::memory::identifiers::PageId;

    PageTable pageTable;
    PhysicalMemoryManager physicalMemory{0};
    FIFOPageReplacementPolicy replacementPolicy;

    const PageSize pageSize{4096};

    MemoryManagementUnit mmu(
        pageTable,
        physicalMemory,
        replacementPolicy,
        pageSize
    );

    const ProcessId processId{100};
    const PageId pageId{0};

    ASSERT_TRUE(
        mmu.registerPage(
            Page{
                pageId,
                processId
            }
        )
    );

    const MemoryAccess access(
        processId,
        VirtualAddress{0},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{1}
    );

    const auto result = mmu.access(access);

    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());

    EXPECT_FALSE(
        result.errorInformation().empty()
    );

    EXPECT_EQ(
        mmu.pageFaultCount(),
        1U
    );

    EXPECT_EQ(
        mmu.pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        mmu.dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.capacity(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_TRUE(
        pageTable.empty()
    );

    const Page* page = mmu.page(pageId);

    ASSERT_NE(page, nullptr);

    EXPECT_FALSE(
        page->isResident()
    );

    EXPECT_FALSE(
        page->mappedFrame().has_value()
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );
}


// ============================================================================
// Repeated Page Faults
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    RepeatedPageFaultsReuseFramesThroughReplacementWithoutLeakingFrames
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    const auto first = mmu->access(
        read(kProcess1, kPage0, 0, 1)
    );

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(first.pageFault());
    EXPECT_FALSE(first.pageReplacement());
    EXPECT_EQ(
        first.frameId(),
        std::optional<FrameId>{kFrame0}
    );

    const auto second = mmu->access(
        read(kProcess1, kPage1, 0, 2)
    );

    ASSERT_TRUE(second.success());
    ASSERT_TRUE(second.pageFault());
    EXPECT_FALSE(second.pageReplacement());
    EXPECT_EQ(
        second.frameId(),
        std::optional<FrameId>{kFrame1}
    );

    const auto third = mmu->access(
        read(kProcess1, kPage2, 0, 3)
    );

    ASSERT_TRUE(third.success());
    ASSERT_TRUE(third.pageFault());
    EXPECT_TRUE(third.pageReplacement());
    EXPECT_EQ(
        third.frameId(),
        std::optional<FrameId>{kFrame0}
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        1U
    );

    const auto resident = mmu->access(
        read(kProcess1, kPage2, 128, 4)
    );

    ASSERT_TRUE(resident.success());
    EXPECT_FALSE(resident.pageFault());
    EXPECT_FALSE(resident.pageReplacement());

    ASSERT_TRUE(resident.physicalAddress().has_value());

    EXPECT_EQ(
        resident.physicalAddress().value(),
        emmus::memory::access::PhysicalAddress{128}
    );

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        1U
    );

    expectIntegratedStateIsConsistent();
}


// ============================================================================
// Reset
// ============================================================================

TEST_F(
    MemoryManagementUnitIntegrationTest,
    ResetRestoresAllIntegratedComponentsToInitialState
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    ASSERT_TRUE(
        mmu->access(
            write(kProcess1, kPage0, 0, 1)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage1, 0, 2)
        ).success()
    );

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage2, 0, 3)
        ).success()
    );

    ASSERT_GT(
        mmu->pageFaultCount(),
        0U
    );

    ASSERT_GT(
        mmu->pageReplacementCount(),
        0U
    );

    ASSERT_GT(
        mmu->dirtyEvictionCount(),
        0U
    );

    ASSERT_GT(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    ASSERT_GT(
        replacementPolicy.statistics().dirtyEvictionCount(),
        0U
    );

    mmu->reset();

    EXPECT_EQ(
        mmu->pageFaultCount(),
        0U
    );

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        2U
    );

    EXPECT_TRUE(
        pageTable.empty()
    );

    EXPECT_EQ(
        replacementPolicy.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        replacementPolicy.statistics().dirtyEvictionCount(),
        0U
    );

    const Page* page0 = mmu->page(kPage0);
    const Page* page1 = mmu->page(kPage1);
    const Page* page2 = mmu->page(kPage2);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page1, nullptr);
    ASSERT_NE(page2, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_FALSE(page1->isResident());
    EXPECT_FALSE(page2->isResident());

    EXPECT_FALSE(page0->isDirty());
    EXPECT_FALSE(page1->isDirty());
    EXPECT_FALSE(page2->isDirty());

    EXPECT_FALSE(page0->isReferenced());
    EXPECT_FALSE(page1->isReferenced());
    EXPECT_FALSE(page2->isReferenced());

    EXPECT_FALSE(page0->mappedFrame().has_value());
    EXPECT_FALSE(page1->mappedFrame().has_value());
    EXPECT_FALSE(page2->mappedFrame().has_value());

    expectIntegratedStateIsConsistent();
}

} // namespace emmus::tests
