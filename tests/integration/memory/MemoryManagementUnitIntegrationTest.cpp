#include <cstdint>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessResult.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/mmu/MemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual/Page.hpp"

namespace
{

using emmus::algorithms::replacement::FIFOPageReplacementPolicy;
using emmus::memory::access::MemoryAccess;
using emmus::memory::access::MemoryAccessOperation;
using emmus::memory::access::MemoryAccessResult;
using emmus::memory::access::PageSize;
using emmus::memory::access::VirtualAddress;
using emmus::memory::identifiers::FrameId;
using emmus::memory::identifiers::PageId;
using emmus::memory::identifiers::ProcessId;
using emmus::memory::mmu::MemoryManagementUnit;
using emmus::memory::mmu::PageTable;
using emmus::memory::physical::PhysicalMemoryManager;
using emmus::memory::virtual_memory::Page;

class MemoryManagementUnitIntegrationTest : public ::testing::Test
{
protected:
    static constexpr std::uint64_t kPageSize = 4096U;

    static constexpr ProcessId kProcess1{100};
    static constexpr ProcessId kProcess2{200};

    static constexpr PageId kPage0{0};
    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};

    static constexpr std::uint64_t kOffset = 128U;

    PageTable pageTable;
    PhysicalMemoryManager physicalMemory{2};

    emmus::memory::mmu::PageTablePhysicalMemoryIntegration integration{
        pageTable,
        physicalMemory
    };

    FIFOPageReplacementPolicy replacementPolicy;
    PageSize pageSize{kPageSize};

    MemoryManagementUnit mmu{
        pageTable,
        physicalMemory,
        replacementPolicy,
        pageSize
    };

    void registerPage(
        PageId pageId,
        ProcessId processId)
    {
        ASSERT_TRUE(
            mmu.registerPage(
                Page{pageId, processId}
            )
        );
    }

    [[nodiscard]]
    static VirtualAddress virtualAddress(
        PageId pageId,
        std::uint64_t offset = kOffset)
    {
        return VirtualAddress{
            static_cast<std::uint64_t>(pageId.value()) *
                kPageSize +
            offset
        };
    }

    [[nodiscard]]
    MemoryAccess read(
        ProcessId processId,
        PageId pageId,
        std::uint64_t sequenceNumber,
        std::uint64_t offset = kOffset) const
    {
        return MemoryAccess{
            processId,
            virtualAddress(pageId, offset),
            MemoryAccessOperation::Read,
            emmus::memory::access::AccessSequenceNumber{
                sequenceNumber
            }
        };
    }

    [[nodiscard]]
    MemoryAccess write(
        ProcessId processId,
        PageId pageId,
        std::uint64_t sequenceNumber,
        std::uint64_t offset = kOffset) const
    {
        return MemoryAccess{
            processId,
            virtualAddress(pageId, offset),
            MemoryAccessOperation::Write,
            emmus::memory::access::AccessSequenceNumber{
                sequenceNumber
            }
        };
    }
};

TEST_F(
    MemoryManagementUnitIntegrationTest,
    FirstAccessLoadsPageIntoPhysicalMemoryAndCreatesPageTableMapping)
{
    registerPage(kPage0, kProcess1);

    const auto result =
        mmu.access(
            read(kProcess1, kPage0, 1U)
        );

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), FrameId{0});

    ASSERT_TRUE(result.physicalAddress().has_value());

    EXPECT_EQ(
        result.physicalAddress().value().value(),
        kOffset
    );

    const auto mappedFrame =
        pageTable.lookup(kPage0);

    ASSERT_TRUE(mappedFrame.has_value());
    EXPECT_EQ(mappedFrame.value(), FrameId{0});

    const auto physicalFrame =
        physicalMemory.frame(FrameId{0});

    ASSERT_NE(physicalFrame, nullptr);
    ASSERT_TRUE(physicalFrame->mappedPage().has_value());
    EXPECT_EQ(
        physicalFrame->mappedPage().value(),
        kPage0
    );

    const auto* page =
        mmu.page(kPage0);

    ASSERT_NE(page, nullptr);
    EXPECT_TRUE(page->isResident());
    ASSERT_TRUE(page->mappedFrame().has_value());
    EXPECT_EQ(
        page->mappedFrame().value(),
        FrameId{0}
    );

    EXPECT_TRUE(page->isReferenced());

    EXPECT_TRUE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_TRUE(
        physicalMemory.isPageMapped(kPage0)
    );
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    ResidentReadUsesExistingMappingWithoutPageFault)
{
    registerPage(kPage0, kProcess1);

    const auto firstResult =
        mmu.access(
            read(kProcess1, kPage0, 1U)
        );

    ASSERT_TRUE(firstResult.success());
    ASSERT_TRUE(firstResult.frameId().has_value());

    const auto expectedFrame =
        firstResult.frameId().value();

    const auto secondResult =
        mmu.access(
            read(kProcess1, kPage0, 2U, 256U)
        );

    ASSERT_TRUE(secondResult.success());
    EXPECT_FALSE(secondResult.pageFault());
    EXPECT_FALSE(secondResult.pageReplacement());
    EXPECT_FALSE(secondResult.dirtyEviction());

    ASSERT_TRUE(secondResult.frameId().has_value());
    EXPECT_EQ(
        secondResult.frameId().value(),
        expectedFrame
    );

    ASSERT_TRUE(secondResult.physicalAddress().has_value());

    EXPECT_EQ(
        secondResult.physicalAddress().value().value(),
        expectedFrame.value() * kPageSize + 256U
    );

    EXPECT_EQ(
        mmu.pageFaultCount(),
        1U
    );

    EXPECT_EQ(
        mmu.pageReplacementCount(),
        0U
    );
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    WriteAccessMarksResidentPageDirty)
{
    registerPage(kPage0, kProcess1);

    const auto result =
        mmu.access(
            write(kProcess1, kPage0, 1U)
        );

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());

    const auto* page =
        mmu.page(kPage0);

    ASSERT_NE(page, nullptr);
    EXPECT_TRUE(page->isResident());
    EXPECT_TRUE(page->isReferenced());
    EXPECT_TRUE(page->isDirty());
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    MultiplePagesOccupyDistinctPhysicalFrames)
{
    registerPage(kPage0, kProcess1);
    registerPage(kPage1, kProcess1);

    const auto result0 =
        mmu.access(
            read(kProcess1, kPage0, 1U)
        );

    const auto result1 =
        mmu.access(
            read(kProcess1, kPage1, 2U)
        );

    ASSERT_TRUE(result0.success());
    ASSERT_TRUE(result1.success());

    ASSERT_TRUE(result0.frameId().has_value());
    ASSERT_TRUE(result1.frameId().has_value());

    EXPECT_NE(
        result0.frameId().value(),
        result1.frameId().value()
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        pageTable.size(),
        2U
    );

    EXPECT_TRUE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_TRUE(
        pageTable.isMapped(kPage1)
    );

    EXPECT_TRUE(
        physicalMemory.isPageMapped(kPage0)
    );

    EXPECT_TRUE(
        physicalMemory.isPageMapped(kPage1)
    );
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    ThirdPageCausesFIFOReplacementWhenPhysicalMemoryIsFull)
{
    registerPage(kPage0, kProcess1);
    registerPage(kPage1, kProcess1);
    registerPage(kPage2, kProcess1);

    const auto first =
        mmu.access(
            read(kProcess1, kPage0, 1U)
        );

    const auto second =
        mmu.access(
            read(kProcess1, kPage1, 2U)
        );

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(second.success());

    ASSERT_TRUE(first.frameId().has_value());
    ASSERT_TRUE(second.frameId().has_value());

    EXPECT_EQ(
        first.frameId().value(),
        FrameId{0}
    );

    EXPECT_EQ(
        second.frameId().value(),
        FrameId{1}
    );

    const auto replacement =
        mmu.access(
            read(kProcess1, kPage2, 3U)
        );

    ASSERT_TRUE(replacement.success());

    EXPECT_TRUE(replacement.pageFault());
    EXPECT_TRUE(replacement.pageReplacement());
    EXPECT_FALSE(replacement.dirtyEviction());

    ASSERT_TRUE(replacement.frameId().has_value());

    EXPECT_EQ(
        replacement.frameId().value(),
        FrameId{0}
    );

    EXPECT_EQ(
        mmu.pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu.pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        mmu.dirtyEvictionCount(),
        0U
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_TRUE(
        pageTable.isMapped(kPage1)
    );

    EXPECT_TRUE(
        pageTable.isMapped(kPage2)
    );

    EXPECT_FALSE(
        physicalMemory.isPageMapped(kPage0)
    );

    EXPECT_TRUE(
        physicalMemory.isPageMapped(kPage1)
    );

    EXPECT_TRUE(
        physicalMemory.isPageMapped(kPage2)
    );

    const auto* page0 =
        mmu.page(kPage0);

    const auto* page1 =
        mmu.page(kPage1);

    const auto* page2 =
        mmu.page(kPage2);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page1, nullptr);
    ASSERT_NE(page2, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_TRUE(page1->isResident());
    EXPECT_TRUE(page2->isResident());

    EXPECT_TRUE(
        integration.isConsistent()
    );
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    DirtyFIFOReplacementReportsDirtyEviction)
{
    registerPage(kPage0, kProcess1);
    registerPage(kPage1, kProcess1);
    registerPage(kPage2, kProcess1);

    const auto first =
        mmu.access(
            write(kProcess1, kPage0, 1U)
        );

    const auto second =
        mmu.access(
            read(kProcess1, kPage1, 2U)
        );

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(second.success());

    const auto replacement =
        mmu.access(
            read(kProcess1, kPage2, 3U)
        );

    ASSERT_TRUE(replacement.success());

    EXPECT_TRUE(replacement.pageFault());
    EXPECT_TRUE(replacement.pageReplacement());
    EXPECT_TRUE(replacement.dirtyEviction());

    EXPECT_EQ(
        mmu.pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        mmu.pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        mmu.dirtyEvictionCount(),
        1U
    );

    const auto* evictedPage =
        mmu.page(kPage0);

    ASSERT_NE(evictedPage, nullptr);

    EXPECT_FALSE(
        evictedPage->isResident()
    );

    EXPECT_FALSE(
        evictedPage->isDirty()
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_TRUE(
        pageTable.isMapped(kPage2)
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    ProcessIsolationPreventsAccessToAnotherProcessesPage)
{
    registerPage(kPage0, kProcess1);

    const auto result =
        mmu.access(
            read(kProcess2, kPage0, 1U)
        );

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());

    EXPECT_EQ(
        mmu.pageFaultCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        pageTable.size(),
        0U
    );
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    PageTableAndPhysicalMemoryRemainConsistentAfterReplacement)
{
    registerPage(kPage0, kProcess1);
    registerPage(kPage1, kProcess1);
    registerPage(kPage2, kProcess1);

    ASSERT_TRUE(
        mmu.access(
            read(kProcess1, kPage0, 1U)
        ).success()
    );

    ASSERT_TRUE(
        mmu.access(
            read(kProcess1, kPage1, 2U)
        ).success()
    );

    ASSERT_TRUE(
        mmu.access(
            read(kProcess1, kPage2, 3U)
        ).success()
    );

    EXPECT_EQ(
        pageTable.size(),
        physicalMemory.allocatedFrameCount()
    );

    for (const auto pageId :
         std::vector<PageId>{kPage0, kPage1, kPage2})
    {
        const auto* page =
            mmu.page(pageId);

        ASSERT_NE(page, nullptr);

        if (!page->isResident())
        {
            EXPECT_FALSE(
                pageTable.isMapped(pageId)
            );

            EXPECT_FALSE(
                physicalMemory.isPageMapped(pageId)
            );

            continue;
        }

        ASSERT_TRUE(
            page->mappedFrame().has_value()
        );

        const auto mappedFrame =
            page->mappedFrame().value();

        const auto tableFrame =
            pageTable.lookup(pageId);

        ASSERT_TRUE(tableFrame.has_value());
        EXPECT_EQ(
            tableFrame.value(),
            mappedFrame
        );

        const auto physicalFrame =
            physicalMemory.frame(mappedFrame);

        ASSERT_NE(physicalFrame, nullptr);
        ASSERT_TRUE(
            physicalFrame->mappedPage().has_value()
        );

        EXPECT_EQ(
            physicalFrame->mappedPage().value(),
            pageId
        );
    }
}

TEST_F(
    MemoryManagementUnitIntegrationTest,
    ResetRestoresAllIntegratedComponentsToInitialState)
{
    registerPage(kPage0, kProcess1);
    registerPage(kPage1, kProcess1);

    ASSERT_TRUE(
        mmu.access(
            write(kProcess1, kPage0, 1U)
        ).success()
    );

    ASSERT_TRUE(
        mmu.access(
            read(kProcess1, kPage1, 2U)
        ).success()
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U
    );

    EXPECT_EQ(
        pageTable.size(),
        2U
    );

    EXPECT_GT(
        mmu.pageFaultCount(),
        0U
    );

    mmu.reset();

    EXPECT_EQ(
        mmu.pageFaultCount(),
        0U
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
        pageTable.size(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        physicalMemory.capacity()
    );

    const auto* page0 =
        mmu.page(kPage0);

    const auto* page1 =
        mmu.page(kPage1);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page1, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_FALSE(page1->isResident());

    EXPECT_FALSE(page0->isDirty());
    EXPECT_FALSE(page1->isDirty());

    EXPECT_FALSE(page0->isReferenced());
    EXPECT_FALSE(page1->isReferenced());
}

} // namespace