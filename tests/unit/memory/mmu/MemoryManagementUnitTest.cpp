#include <cstddef>
#include <cstdint>
#include <memory>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessResult.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/mmu/MemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual_memory/Page.hpp"

namespace
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

using FrameId =
    emmus::memory::identifiers::FrameId;


/**
 * @brief Replacement-policy test double that records MMU notifications.
 */
class RecordingReplacementPolicy final
    : public emmus::algorithms::replacement::IPageReplacementPolicy
{
public:
    struct LoadEvent
    {
        PageId pageId;
        FrameId frameId;
    };

    struct AccessEvent
    {
        PageId pageId;
        FrameId frameId;
    };

    struct RemovalEvent
    {
        PageId pageId;
        FrameId frameId;
    };

    void pageLoaded(
        PageId pageId,
        FrameId frameId) override
    {
        loadedEvents.push_back({pageId, frameId});
    }

    void pageAccessed(
        PageId pageId,
        FrameId frameId) override
    {
        accessedEvents.push_back({pageId, frameId});
    }

    void pageRemoved(
        PageId pageId,
        FrameId frameId) override
    {
        removedEvents.push_back({pageId, frameId});
    }

    void recordDirtyEviction() noexcept override
    {
        ++dirtyEvictionCallCount;
    }

    std::optional<FrameId> chooseVictim() override
    {
        ++chooseVictimCallCount;
        return nextVictim;
    }

    void reset() override
    {
        loadedEvents.clear();
        accessedEvents.clear();
        removedEvents.clear();

        nextVictim.reset();

        chooseVictimCallCount = 0;
        dirtyEvictionCallCount = 0;
    }

    std::optional<FrameId> nextVictim;

    std::size_t chooseVictimCallCount{0};

    std::size_t dirtyEvictionCallCount{0};

    std::vector<LoadEvent> loadedEvents;

    std::vector<AccessEvent> accessedEvents;

    std::vector<RemovalEvent> removedEvents;
};


class MemoryManagementUnitTest : public ::testing::Test
{
protected:
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
            pageSize);
    }

    MemoryAccess read(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) *
                    kPageSize +
                offset},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{sequence});
    }

    MemoryAccess write(
        ProcessId processId,
        PageId pageId,
        std::uint64_t offset = 0,
        std::uint64_t sequence = 0) const
    {
        return MemoryAccess(
            processId,
            VirtualAddress{
                static_cast<std::uint64_t>(pageId.value()) *
                    kPageSize +
                offset},
            MemoryAccessOperation::Write,
            AccessSequenceNumber{sequence});
    }

    PageTable pageTable;

    PhysicalMemoryManager physicalMemory{2};

    RecordingReplacementPolicy replacementPolicy;

    PageSize pageSize{kPageSize};

    std::unique_ptr<MemoryManagementUnit> mmu;
};


TEST_F(
    MemoryManagementUnitTest,
    StartsWithEmptyPageRegistryAndZeroCounters)
{
    EXPECT_EQ(mmu->registeredPageCount(), 0U);
    EXPECT_EQ(mmu->pageFaultCount(), 0U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);
    EXPECT_EQ(mmu->dirtyEvictionCount(), 0U);
    EXPECT_EQ(mmu->pageSize(), pageSize.value());

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    RegistersNonresidentPage)
{
    EXPECT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    EXPECT_EQ(
        mmu->registeredPageCount(),
        1U);

    const Page* page =
        mmu->page(kPage0);

    ASSERT_NE(page, nullptr);

    EXPECT_EQ(
        page->id(),
        kPage0);

    EXPECT_EQ(
        page->processId(),
        kProcess1);

    EXPECT_FALSE(
        page->isResident());
}


/**
 * @brief US-902: A valid registered page with no resident mapping is
 * identified as a page fault rather than an invalid access.
 */
TEST_F(
    MemoryManagementUnitTest,
    ValidNonresidentPageIsDetectedAsPageFault)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                128,
                1));

    EXPECT_TRUE(
        result.success());

    EXPECT_TRUE(
        result.pageFault());

    EXPECT_FALSE(
        result.pageReplacement());

    EXPECT_FALSE(
        result.dirtyEviction());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_EQ(
        result.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        result.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{128}});
}


/**
 * @brief US-902: Once a fault has been resolved, the same page is
 * resident and the following access must not generate another fault.
 */
TEST_F(
    MemoryManagementUnitTest,
    ResolvedPageFaultMakesPageResidentForSubsequentAccess)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto first =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                100,
                1));

    ASSERT_TRUE(
        first.success());

    EXPECT_TRUE(
        first.pageFault());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);

    const auto second =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                200,
                2));

    EXPECT_TRUE(
        second.success());

    EXPECT_FALSE(
        second.pageFault());

    EXPECT_FALSE(
        second.pageReplacement());

    EXPECT_FALSE(
        second.dirtyEviction());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);

    EXPECT_EQ(
        second.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        second.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{200}});
}


/**
 * @brief US-902: A faulting read preserves the read operation semantics.
 * The resolved page is referenced but remains clean.
 */
TEST_F(
    MemoryManagementUnitTest,
    FaultingReadPreservesReadSemantics)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                321,
                1));

    ASSERT_TRUE(
        result.success());

    EXPECT_TRUE(
        result.pageFault());

    EXPECT_FALSE(
        result.pageReplacement());

    EXPECT_FALSE(
        result.dirtyEviction());

    const Page* page =
        mmu->page(kPage0);

    ASSERT_NE(
        page,
        nullptr);

    EXPECT_TRUE(
        page->isResident());

    EXPECT_TRUE(
        page->isReferenced());

    EXPECT_FALSE(
        page->isDirty());

    EXPECT_EQ(
        replacementPolicy.loadedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].frameId,
        kFrame0);
}


/**
 * @brief US-902: A faulting write preserves the write operation semantics.
 * The resolved page is referenced and marked dirty.
 */
TEST_F(
    MemoryManagementUnitTest,
    FaultingWritePreservesWriteSemantics)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            write(
                kProcess1,
                kPage0,
                654,
                1));

    ASSERT_TRUE(
        result.success());

    EXPECT_TRUE(
        result.pageFault());

    EXPECT_FALSE(
        result.pageReplacement());

    EXPECT_FALSE(
        result.dirtyEviction());

    const Page* page =
        mmu->page(kPage0);

    ASSERT_NE(
        page,
        nullptr);

    EXPECT_TRUE(
        page->isResident());

    EXPECT_TRUE(
        page->isReferenced());

    EXPECT_TRUE(
        page->isDirty());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].frameId,
        kFrame0);
}


/**
 * @brief US-902: The page-fault path must handle accesses at the beginning
 * and end of a virtual page and at the beginning and end of the next page.
 */
TEST_F(
    MemoryManagementUnitTest,
    PageBoundaryAddressesResolveThroughPageFaultPath)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    const auto first =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1));

    ASSERT_TRUE(
        first.success());

    EXPECT_TRUE(
        first.pageFault());

    EXPECT_EQ(
        first.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        first.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{0}});

    const auto second =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                kPageSize - 1,
                2));

    ASSERT_TRUE(
        second.success());

    EXPECT_FALSE(
        second.pageFault());

    EXPECT_EQ(
        second.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        second.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{
                kPageSize - 1}});

    const auto third =
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                3));

    ASSERT_TRUE(
        third.success());

    EXPECT_TRUE(
        third.pageFault());

    EXPECT_EQ(
        third.frameId(),
        std::optional<FrameId>{kFrame1});

    EXPECT_EQ(
        third.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{
                kPageSize}});

    const auto fourth =
        mmu->access(
            read(
                kProcess1,
                kPage1,
                kPageSize - 1,
                4));

    ASSERT_TRUE(
        fourth.success());

    EXPECT_FALSE(
        fourth.pageFault());

    EXPECT_EQ(
        fourth.frameId(),
        std::optional<FrameId>{kFrame1});

    EXPECT_EQ(
        fourth.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{
                2 * kPageSize - 1}});

    EXPECT_EQ(
        mmu->pageFaultCount(),
        2U);
}


TEST_F(
    MemoryManagementUnitTest,
    RejectsDuplicatePageRegistration)
{
    EXPECT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    EXPECT_FALSE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    EXPECT_EQ(
        mmu->registeredPageCount(),
        1U);
}


TEST_F(
    MemoryManagementUnitTest,
    RejectsRegistrationOfAlreadyResidentPage)
{
    Page page{
        kPage0,
        kProcess1};

    ASSERT_FALSE(
        page.mapToFrame(kFrame0).has_value());

    ASSERT_TRUE(
        page.isResident());

    EXPECT_FALSE(
        mmu->registerPage(
            std::move(page)));

    EXPECT_EQ(
        mmu->registeredPageCount(),
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    FirstReadCausesPageFaultLoadsPageAndTranslatesAddress)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                123,
                1));

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    ASSERT_TRUE(
        result.frameId().has_value());

    EXPECT_EQ(
        result.frameId().value(),
        kFrame0);

    ASSERT_TRUE(
        result.physicalAddress().has_value());

    EXPECT_EQ(
        result.physicalAddress().value(),
        emmus::memory::access::PhysicalAddress{123});

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    const Page* page =
        mmu->page(kPage0);

    ASSERT_NE(page, nullptr);

    EXPECT_TRUE(
        page->isResident());

    ASSERT_TRUE(
        page->mappedFrame().has_value());

    EXPECT_EQ(
        page->mappedFrame().value(),
        kFrame0);

    EXPECT_TRUE(
        page->isReferenced());

    EXPECT_FALSE(
        page->isDirty());

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0});

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].frameId,
        kFrame0);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    InitialPageFaultNotifiesPolicyLoadedBeforeAccessed)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1));

    ASSERT_TRUE(
        result.success());

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        1U);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    ResidentReadDoesNotCausePageFault)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage0,
                10,
                1))
        .success());

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        1U);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        1U);

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                20,
                2));

    EXPECT_TRUE(
        result.success());

    EXPECT_FALSE(
        result.pageFault());

    EXPECT_FALSE(
        result.pageReplacement());

    EXPECT_FALSE(
        result.dirtyEviction());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        2U);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[1].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[1].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.size(),
        1U);
}


TEST_F(
    MemoryManagementUnitTest,
    ResidentWriteMarksPageDirtyAndReferenced)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            write(
                kProcess1,
                kPage0,
                64,
                1));

    EXPECT_TRUE(
        result.success());

    EXPECT_TRUE(
        result.pageFault());

    const Page* page =
        mmu->page(kPage0);

    ASSERT_NE(
        page,
        nullptr);

    EXPECT_TRUE(
        page->isResident());

    EXPECT_TRUE(
        page->isReferenced());

    EXPECT_TRUE(
        page->isDirty());

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        1U);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    MultiplePagesUseDistinctFreeFrames)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    const auto first =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1));

    const auto second =
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                2));

    ASSERT_TRUE(first.success());
    ASSERT_TRUE(second.success());

    ASSERT_TRUE(
        first.frameId().has_value());

    ASSERT_TRUE(
        second.frameId().has_value());

    EXPECT_EQ(
        first.frameId().value(),
        kFrame0);

    EXPECT_EQ(
        second.frameId().value(),
        kFrame1);

    EXPECT_EQ(
        mmu->pageFaultCount(),
        2U);

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U);

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        2U);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        2U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[1].pageId,
        kPage1);

    EXPECT_EQ(
        replacementPolicy.loadedEvents[1].frameId,
        kFrame1);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[1].pageId,
        kPage1);

    EXPECT_EQ(
        replacementPolicy.accessedEvents[1].frameId,
        kFrame1);
}


TEST_F(
    MemoryManagementUnitTest,
    FreeFramesAreAllocatedBeforeReplacementPolicyIsInvoked)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage2, kProcess1}));

    replacementPolicy.nextVictim =
        kFrame1;

    const auto first =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1));

    ASSERT_TRUE(
        first.success());

    EXPECT_TRUE(
        first.pageFault());

    EXPECT_FALSE(
        first.pageReplacement());

    EXPECT_EQ(
        first.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    const auto second =
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                2));

    ASSERT_TRUE(
        second.success());

    EXPECT_TRUE(
        second.pageFault());

    EXPECT_FALSE(
        second.pageReplacement());

    EXPECT_EQ(
        second.frameId(),
        std::optional<FrameId>{kFrame1});

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);

    EXPECT_EQ(
        mmu->pageFaultCount(),
        2U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U);

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U);

    const auto third =
        mmu->access(
            read(
                kProcess1,
                kPage2,
                0,
                3));

    ASSERT_TRUE(
        third.success());

    EXPECT_TRUE(
        third.pageFault());

    EXPECT_TRUE(
        third.pageReplacement());

    EXPECT_EQ(
        third.frameId(),
        std::optional<FrameId>{kFrame1});

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        1U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U);
}


TEST_F(
    MemoryManagementUnitTest,
    FullMemoryCausesReplacementUsingPolicyVictim)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage2, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1))
        .success());

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                2))
        .success());

    replacementPolicy.nextVictim =
        kFrame0;

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage2,
                0,
                3));

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    ASSERT_TRUE(
        result.frameId().has_value());

    EXPECT_EQ(
        result.frameId().value(),
        kFrame0);

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        1U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);

    const Page* victim =
        mmu->page(kPage0);

    ASSERT_NE(
        victim,
        nullptr);

    EXPECT_FALSE(
        victim->isResident());

    const Page* requested =
        mmu->page(kPage2);

    ASSERT_NE(
        requested,
        nullptr);

    EXPECT_TRUE(
        requested->isResident());

    ASSERT_TRUE(
        requested->mappedFrame().has_value());

    EXPECT_EQ(
        requested->mappedFrame().value(),
        kFrame0);

    EXPECT_FALSE(
        pageTable.isMapped(kPage0));

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage2),
        std::optional<FrameId>{kFrame0});

    ASSERT_EQ(
        replacementPolicy.removedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.removedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.removedEvents[0].frameId,
        kFrame0);

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        3U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.back().pageId,
        kPage2);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.back().frameId,
        kFrame0);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        3U);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.back().pageId,
        kPage2);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.back().frameId,
        kFrame0);
}


/**
 * @brief US-902: After a page has been evicted, accessing it again must
 * be recognized as a new page fault and successfully resolve the access.
 */
TEST_F(
    MemoryManagementUnitTest,
    AccessToPreviouslyEvictedPageCausesNewPageFault)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage2, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage0,
                10,
                1))
        .success());

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage1,
                20,
                2))
        .success());

    replacementPolicy.nextVictim =
        kFrame0;

    const auto evictPage0 =
        mmu->access(
            read(
                kProcess1,
                kPage2,
                30,
                3));

    ASSERT_TRUE(
        evictPage0.success());

    EXPECT_TRUE(
        evictPage0.pageFault());

    EXPECT_TRUE(
        evictPage0.pageReplacement());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U);

    const Page* page0AfterEviction =
        mmu->page(kPage0);

    ASSERT_NE(
        page0AfterEviction,
        nullptr);

    EXPECT_FALSE(
        page0AfterEviction->isResident());

    EXPECT_FALSE(
        pageTable.isMapped(kPage0));

    EXPECT_FALSE(
        physicalMemory.frameForPage(kPage0).has_value());

    const auto reloadPage0 =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                40,
                4));

    ASSERT_TRUE(
        reloadPage0.success());

    EXPECT_TRUE(
        reloadPage0.pageFault());

    EXPECT_TRUE(
        reloadPage0.pageReplacement());

    EXPECT_EQ(
        reloadPage0.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        mmu->pageFaultCount(),
        4U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        2U);

    const Page* page0AfterReload =
        mmu->page(kPage0);

    ASSERT_NE(
        page0AfterReload,
        nullptr);

    EXPECT_TRUE(
        page0AfterReload->isResident());

    EXPECT_TRUE(
        page0AfterReload->isReferenced());

    EXPECT_FALSE(
        page0AfterReload->isDirty());

    EXPECT_EQ(
        page0AfterReload->mappedFrame(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0});

    const auto residentAccess =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                50,
                5));

    ASSERT_TRUE(
        residentAccess.success());

    EXPECT_FALSE(
        residentAccess.pageFault());

    EXPECT_FALSE(
        residentAccess.pageReplacement());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        4U);
}


TEST_F(
    MemoryManagementUnitTest,
    ReplacementNotifiesPolicyInRemovalLoadAccessOrder)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage2, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1))
        .success());

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                2))
        .success());

    replacementPolicy.nextVictim =
        kFrame0;

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage2,
                0,
                3));

    ASSERT_TRUE(
        result.success());

    ASSERT_TRUE(
        result.pageReplacement());

    ASSERT_EQ(
        replacementPolicy.removedEvents.size(),
        1U);

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        3U);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        3U);

    EXPECT_EQ(
        replacementPolicy.removedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.removedEvents[0].frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.back().pageId,
        kPage2);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.back().frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.back().pageId,
        kPage2);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.back().frameId,
        kFrame0);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    DirtyVictimProducesDirtyEviction)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage2, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            write(
                kProcess1,
                kPage0,
                0,
                1))
        .success());

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                2))
        .success());

    replacementPolicy.nextVictim =
        kFrame0;

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage2,
                0,
                3));

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());
    EXPECT_TRUE(result.dirtyEviction());

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        1U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        1U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        1U);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        1U);

    const Page* victim =
        mmu->page(kPage0);

    ASSERT_NE(
        victim,
        nullptr);

    EXPECT_FALSE(
        victim->isResident());

    EXPECT_FALSE(
        victim->isDirty());

    EXPECT_FALSE(
        victim->isReferenced());

    const Page* requested =
        mmu->page(kPage2);

    ASSERT_NE(
        requested,
        nullptr);

    EXPECT_TRUE(
        requested->isResident());

    EXPECT_FALSE(
        requested->isDirty());

    EXPECT_TRUE(
        requested->isReferenced());

    ASSERT_EQ(
        replacementPolicy.removedEvents.size(),
        1U);

    EXPECT_EQ(
        replacementPolicy.removedEvents[0].pageId,
        kPage0);

    EXPECT_EQ(
        replacementPolicy.removedEvents[0].frameId,
        kFrame0);

    ASSERT_EQ(
        replacementPolicy.loadedEvents.size(),
        3U);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.back().pageId,
        kPage2);

    EXPECT_EQ(
        replacementPolicy.loadedEvents.back().frameId,
        kFrame0);

    ASSERT_EQ(
        replacementPolicy.accessedEvents.size(),
        3U);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.back().pageId,
        kPage2);

    EXPECT_EQ(
        replacementPolicy.accessedEvents.back().frameId,
        kFrame0);
}


/**
 * @brief US-902: Invalid/unregistered accesses are not page faults.
 */
TEST_F(
    MemoryManagementUnitTest,
    InvalidAccessIsDistinctFromValidNonresidentPageFault)
{
    const auto invalid =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1));

    EXPECT_FALSE(
        invalid.success());

    EXPECT_FALSE(
        invalid.pageFault());

    EXPECT_FALSE(
        invalid.pageReplacement());

    EXPECT_FALSE(
        invalid.dirtyEviction());

    EXPECT_FALSE(
        invalid.frameId().has_value());

    EXPECT_FALSE(
        invalid.physicalAddress().has_value());

    EXPECT_FALSE(
        invalid.errorInformation().empty());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        0U);

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto validNonresident =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                2));

    EXPECT_TRUE(
        validNonresident.success());

    EXPECT_TRUE(
        validNonresident.pageFault());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);
}

// ============================================================================
// US-903: Safe Invalid Memory Access Handling
// ============================================================================

TEST_F(MemoryManagementUnitTest, InvalidReadDoesNotModifyMMUStateOrInvokePager)
{
    ASSERT_TRUE(mmu->registerPage(Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(read(kProcess1, kPage1, 0, 1));

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(mmu->pageFaultCount(), 0U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);
    EXPECT_EQ(mmu->dirtyEvictionCount(), 0U);

    EXPECT_TRUE(pageTable.empty());
    EXPECT_EQ(physicalMemory.allocatedFrameCount(), 0U);
    EXPECT_EQ(physicalMemory.freeFrameCount(), 2U);

    const Page* page0 = mmu->page(kPage0);
    ASSERT_NE(page0, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_FALSE(page0->isReferenced());
    EXPECT_FALSE(page0->isDirty());
    EXPECT_FALSE(page0->mappedFrame().has_value());

    EXPECT_EQ(mmu->page(kPage1), nullptr);

    EXPECT_TRUE(replacementPolicy.loadedEvents.empty());
    EXPECT_TRUE(replacementPolicy.accessedEvents.empty());
    EXPECT_TRUE(replacementPolicy.removedEvents.empty());
    EXPECT_EQ(replacementPolicy.chooseVictimCallCount, 0U);
    EXPECT_EQ(replacementPolicy.dirtyEvictionCallCount, 0U);
}


TEST_F(MemoryManagementUnitTest, InvalidWriteDoesNotModifyMMUStateOrInvokePager)
{
    ASSERT_TRUE(mmu->registerPage(Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(write(kProcess1, kPage1, 0, 1));

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(mmu->pageFaultCount(), 0U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);
    EXPECT_EQ(mmu->dirtyEvictionCount(), 0U);

    EXPECT_TRUE(pageTable.empty());
    EXPECT_EQ(physicalMemory.allocatedFrameCount(), 0U);
    EXPECT_EQ(physicalMemory.freeFrameCount(), 2U);

    const Page* page0 = mmu->page(kPage0);
    ASSERT_NE(page0, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_FALSE(page0->isReferenced());
    EXPECT_FALSE(page0->isDirty());
    EXPECT_FALSE(page0->mappedFrame().has_value());

    EXPECT_EQ(mmu->page(kPage1), nullptr);

    EXPECT_TRUE(replacementPolicy.loadedEvents.empty());
    EXPECT_TRUE(replacementPolicy.accessedEvents.empty());
    EXPECT_TRUE(replacementPolicy.removedEvents.empty());
    EXPECT_EQ(replacementPolicy.chooseVictimCallCount, 0U);
    EXPECT_EQ(replacementPolicy.dirtyEvictionCallCount, 0U);
}


TEST_F(
    MemoryManagementUnitTest,
    MaximumVirtualAddressIsRejectedWhenItsPageIsUnregistered)
{
    ASSERT_TRUE(mmu->registerPage(Page{kPage0, kProcess1}));

    const MemoryAccess invalidAccess(
        kProcess1,
        VirtualAddress{
            std::numeric_limits<std::uint64_t>::max()},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{1});

    const auto result =
        mmu->access(invalidAccess);

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(mmu->pageFaultCount(), 0U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);
    EXPECT_EQ(mmu->dirtyEvictionCount(), 0U);

    EXPECT_TRUE(pageTable.empty());
    EXPECT_EQ(physicalMemory.allocatedFrameCount(), 0U);
    EXPECT_EQ(physicalMemory.freeFrameCount(), 2U);

    const Page* page0 = mmu->page(kPage0);
    ASSERT_NE(page0, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_FALSE(page0->isReferenced());
    EXPECT_FALSE(page0->isDirty());
    EXPECT_FALSE(page0->mappedFrame().has_value());

    EXPECT_TRUE(replacementPolicy.loadedEvents.empty());
    EXPECT_TRUE(replacementPolicy.accessedEvents.empty());
    EXPECT_TRUE(replacementPolicy.removedEvents.empty());
    EXPECT_EQ(replacementPolicy.chooseVictimCallCount, 0U);
}


TEST_F(
    MemoryManagementUnitTest,
    InvalidAccessDoesNotInvokeReplacementWhenPhysicalMemoryIsFull)
{
    ASSERT_TRUE(mmu->registerPage(Page{kPage0, kProcess1}));
    ASSERT_TRUE(mmu->registerPage(Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(kProcess1, kPage0, 0, 1))
        .success());

    ASSERT_TRUE(
        mmu->access(
            write(kProcess1, kPage1, 0, 2))
        .success());

    ASSERT_EQ(physicalMemory.allocatedFrameCount(), 2U);
    ASSERT_EQ(physicalMemory.freeFrameCount(), 0U);
    ASSERT_EQ(mmu->pageFaultCount(), 2U);
    ASSERT_EQ(mmu->pageReplacementCount(), 0U);
    ASSERT_EQ(mmu->dirtyEvictionCount(), 0U);

    const auto result =
        mmu->access(
            read(kProcess1, kPage2, 0, 3));

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(mmu->pageFaultCount(), 2U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);
    EXPECT_EQ(mmu->dirtyEvictionCount(), 0U);

    EXPECT_EQ(replacementPolicy.chooseVictimCallCount, 0U);
    EXPECT_EQ(replacementPolicy.dirtyEvictionCallCount, 0U);

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::optional<FrameId>{kFrame1});

    EXPECT_FALSE(pageTable.lookup(kPage2).has_value());

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        std::optional<FrameId>{kFrame1});

    EXPECT_FALSE(
        physicalMemory.frameForPage(kPage2).has_value());

    const Page* page0 = mmu->page(kPage0);
    const Page* page1 = mmu->page(kPage1);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page1, nullptr);

    EXPECT_TRUE(page0->isResident());
    EXPECT_FALSE(page0->isDirty());

    EXPECT_TRUE(page1->isResident());
    EXPECT_TRUE(page1->isDirty());

    EXPECT_EQ(mmu->page(kPage2), nullptr);
}


TEST_F(
    MemoryManagementUnitTest,
    RepeatedInvalidWritesDoNotAccumulateStateChanges)
{
    ASSERT_TRUE(mmu->registerPage(Page{kPage0, kProcess1}));

    constexpr std::uint64_t kInvalidAccessCount = 5;

    for (std::uint64_t sequence = 1;
         sequence <= kInvalidAccessCount;
         ++sequence)
    {
        const auto result =
            mmu->access(
                write(
                    kProcess1,
                    kPage1,
                    0,
                    sequence));

        EXPECT_FALSE(result.success());
        EXPECT_FALSE(result.pageFault());
        EXPECT_FALSE(result.pageReplacement());
        EXPECT_FALSE(result.dirtyEviction());
        EXPECT_FALSE(result.frameId().has_value());
        EXPECT_FALSE(result.physicalAddress().has_value());
        EXPECT_FALSE(result.errorInformation().empty());
    }

    EXPECT_EQ(mmu->pageFaultCount(), 0U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);
    EXPECT_EQ(mmu->dirtyEvictionCount(), 0U);

    EXPECT_TRUE(pageTable.empty());
    EXPECT_EQ(physicalMemory.allocatedFrameCount(), 0U);
    EXPECT_EQ(physicalMemory.freeFrameCount(), 2U);

    const Page* page0 = mmu->page(kPage0);
    ASSERT_NE(page0, nullptr);

    EXPECT_FALSE(page0->isResident());
    EXPECT_FALSE(page0->isReferenced());
    EXPECT_FALSE(page0->isDirty());

    EXPECT_TRUE(replacementPolicy.loadedEvents.empty());
    EXPECT_TRUE(replacementPolicy.accessedEvents.empty());
    EXPECT_TRUE(replacementPolicy.removedEvents.empty());
    EXPECT_EQ(replacementPolicy.chooseVictimCallCount, 0U);
}


TEST_F(
    MemoryManagementUnitTest,
    InvalidAccessDoesNotPreventSubsequentValidAccess)
{
    ASSERT_TRUE(mmu->registerPage(Page{kPage0, kProcess1}));

    const auto invalidResult =
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                1));

    EXPECT_FALSE(invalidResult.success());
    EXPECT_FALSE(invalidResult.pageFault());
    EXPECT_FALSE(invalidResult.pageReplacement());
    EXPECT_FALSE(invalidResult.dirtyEviction());
    EXPECT_FALSE(invalidResult.frameId().has_value());
    EXPECT_FALSE(invalidResult.physicalAddress().has_value());

    EXPECT_EQ(mmu->pageFaultCount(), 0U);
    EXPECT_EQ(physicalMemory.allocatedFrameCount(), 0U);

    const auto validResult =
        mmu->access(
            write(
                kProcess1,
                kPage0,
                321,
                2));

    ASSERT_TRUE(validResult.success());
    EXPECT_TRUE(validResult.pageFault());
    EXPECT_FALSE(validResult.pageReplacement());
    EXPECT_FALSE(validResult.dirtyEviction());

    EXPECT_EQ(
        validResult.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        validResult.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{321}});

    EXPECT_EQ(mmu->pageFaultCount(), 1U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);
    EXPECT_EQ(mmu->dirtyEvictionCount(), 0U);

    EXPECT_EQ(physicalMemory.allocatedFrameCount(), 1U);
    EXPECT_EQ(physicalMemory.freeFrameCount(), 1U);

    const Page* page0 = mmu->page(kPage0);
    ASSERT_NE(page0, nullptr);

    EXPECT_TRUE(page0->isResident());
    EXPECT_TRUE(page0->isReferenced());
    EXPECT_TRUE(page0->isDirty());

    EXPECT_EQ(
        page0->mappedFrame(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_FALSE(pageTable.lookup(kPage1).has_value());

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_FALSE(
        physicalMemory.frameForPage(kPage1).has_value());

    EXPECT_EQ(replacementPolicy.chooseVictimCallCount, 0U);
    EXPECT_EQ(replacementPolicy.dirtyEvictionCallCount, 0U);
}


TEST_F(
    MemoryManagementUnitTest,
    RegisteredNonresidentPageRemainsAValidPageFault)
{
    ASSERT_TRUE(mmu->registerPage(Page{kPage0, kProcess1}));

    const Page* before = mmu->page(kPage0);

    ASSERT_NE(before, nullptr);
    EXPECT_FALSE(before->isResident());
    EXPECT_FALSE(before->mappedFrame().has_value());

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                17,
                1));

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    EXPECT_EQ(
        result.frameId(),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        result.physicalAddress(),
        std::optional<
            emmus::memory::access::PhysicalAddress>{
            emmus::memory::access::PhysicalAddress{17}});

    EXPECT_EQ(mmu->pageFaultCount(), 1U);
    EXPECT_EQ(mmu->pageReplacementCount(), 0U);

    const Page* after = mmu->page(kPage0);

    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->isResident());
    EXPECT_TRUE(after->isReferenced());
    EXPECT_FALSE(after->isDirty());

    EXPECT_EQ(
        after->mappedFrame(),
        std::optional<FrameId>{kFrame0});
}

TEST_F(
    MemoryManagementUnitTest,
    WrongProcessCannotAccessRegisteredPage)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            read(
                kProcess2,
                kPage0,
                0,
                1));

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        0U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_TRUE(
        replacementPolicy.loadedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.accessedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.removedEvents.empty());

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    UnregisteredPageAccessFails)
{
    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1));

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        0U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_TRUE(
        replacementPolicy.loadedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.accessedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.removedEvents.empty());

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


TEST_F(
    MemoryManagementUnitTest,
    FailedAccessDoesNotNotifyReplacementPolicy)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    const auto result =
        mmu->access(
            read(
                kProcess2,
                kPage0,
                0,
                1));

    ASSERT_FALSE(
        result.success());

    EXPECT_TRUE(
        replacementPolicy.loadedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.accessedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.removedEvents.empty());

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


/**
 * @brief Verifies that a resident access fails when the coordinated
 * page-table/physical-memory mapping is no longer consistent.
 */
TEST_F(
    MemoryManagementUnitTest,
    ResidentAccessFailsWhenMappingIsInconsistent)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1))
        .success());

    ASSERT_TRUE(
        pageTable.unmap(kPage0));

    EXPECT_FALSE(
        pageTable.lookup(kPage0).has_value());

    /*
     * The physical-memory mapping still exists, so the lower layers are
     * intentionally inconsistent.
     */
    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0});

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                2));

    EXPECT_FALSE(
        result.success());

    EXPECT_FALSE(
        result.pageFault());

    EXPECT_FALSE(
        result.pageReplacement());

    EXPECT_FALSE(
        result.dirtyEviction());

    EXPECT_FALSE(
        result.frameId().has_value());

    EXPECT_FALSE(
        result.physicalAddress().has_value());

    EXPECT_FALSE(
        result.errorInformation().empty());

    EXPECT_EQ(
        mmu->pageFaultCount(),
        1U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


/**
 * @brief Verifies that a full physical memory cannot be replaced when
 * the active replacement policy supplies no victim frame.
 */
TEST_F(
    MemoryManagementUnitTest,
    FullMemoryAccessFailsWhenReplacementPolicyProvidesNoVictim)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage2, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage0,
                0,
                1))
        .success());

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                2))
        .success());

    /*
     * nextVictim intentionally remains empty.
     */
    EXPECT_FALSE(
        replacementPolicy.nextVictim.has_value());

    const auto result =
        mmu->access(
            read(
                kProcess1,
                kPage2,
                0,
                3));

    EXPECT_FALSE(
        result.success());

    EXPECT_TRUE(
        result.pageFault());

    EXPECT_FALSE(
        result.pageReplacement());

    EXPECT_FALSE(
        result.dirtyEviction());

    EXPECT_FALSE(
        result.frameId().has_value());

    EXPECT_FALSE(
        result.physicalAddress().has_value());

    EXPECT_FALSE(
        result.errorInformation().empty());

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        1U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);

    EXPECT_EQ(
        mmu->pageFaultCount(),
        3U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        2U);

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U);

    const Page* page0 =
        mmu->page(kPage0);

    const Page* page1 =
        mmu->page(kPage1);

    const Page* page2 =
        mmu->page(kPage2);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page1, nullptr);
    ASSERT_NE(page2, nullptr);

    EXPECT_TRUE(
        page0->isResident());

    EXPECT_TRUE(
        page1->isResident());

    EXPECT_FALSE(
        page2->isResident());

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::optional<FrameId>{kFrame1});

    EXPECT_FALSE(
        pageTable.lookup(kPage2).has_value());

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        std::optional<FrameId>{kFrame0});

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        std::optional<FrameId>{kFrame1});

    EXPECT_FALSE(
        physicalMemory.frameForPage(kPage2).has_value());
}


TEST_F(
    MemoryManagementUnitTest,
    ResetUnmapsResidentPagesClearsCountersAndResetsPolicy)
{
    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage0, kProcess1}));

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage1, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            write(
                kProcess1,
                kPage0,
                0,
                1))
        .success());

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage1,
                0,
                2))
        .success());

    replacementPolicy.nextVictim =
        kFrame0;

    ASSERT_TRUE(
        mmu->registerPage(
            Page{kPage2, kProcess1}));

    ASSERT_TRUE(
        mmu->access(
            read(
                kProcess1,
                kPage2,
                0,
                3))
        .success());

    ASSERT_GT(
        mmu->pageFaultCount(),
        0U);

    ASSERT_GT(
        mmu->pageReplacementCount(),
        0U);

    ASSERT_GT(
        mmu->dirtyEvictionCount(),
        0U);

    ASSERT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        1U);

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        1U);

    EXPECT_FALSE(
        replacementPolicy.loadedEvents.empty());

    EXPECT_FALSE(
        replacementPolicy.accessedEvents.empty());

    EXPECT_FALSE(
        replacementPolicy.removedEvents.empty());

    mmu->reset();

    EXPECT_EQ(
        mmu->pageFaultCount(),
        0U);

    EXPECT_EQ(
        mmu->pageReplacementCount(),
        0U);

    EXPECT_EQ(
        mmu->dirtyEvictionCount(),
        0U);

    EXPECT_EQ(
        mmu->registeredPageCount(),
        3U);

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U);

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        2U);

    EXPECT_TRUE(
        pageTable.empty());

    const Page* page0 =
        mmu->page(kPage0);

    const Page* page1 =
        mmu->page(kPage1);

    const Page* page2 =
        mmu->page(kPage2);

    ASSERT_NE(page0, nullptr);
    ASSERT_NE(page1, nullptr);
    ASSERT_NE(page2, nullptr);

    EXPECT_FALSE(
        page0->isResident());

    EXPECT_FALSE(
        page1->isResident());

    EXPECT_FALSE(
        page2->isResident());

    EXPECT_FALSE(
        page0->isDirty());

    EXPECT_FALSE(
        page1->isDirty());

    EXPECT_FALSE(
        page2->isDirty());

    EXPECT_FALSE(
        page0->isReferenced());

    EXPECT_FALSE(
        page1->isReferenced());

    EXPECT_FALSE(
        page2->isReferenced());

    EXPECT_TRUE(
        replacementPolicy.loadedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.accessedEvents.empty());

    EXPECT_TRUE(
        replacementPolicy.removedEvents.empty());

    EXPECT_FALSE(
        replacementPolicy.nextVictim.has_value());

    EXPECT_EQ(
        replacementPolicy.chooseVictimCallCount,
        0U);

    EXPECT_EQ(
        replacementPolicy.dirtyEvictionCallCount,
        0U);
}


// ============================================================================
// US-802: Zero Physical Frames
// ============================================================================

TEST(
    MemoryManagementUnitZeroFrameTest,
    PageFaultFailsWhenPhysicalMemoryContainsZeroFrames)
{
    PageTable pageTable;

    PhysicalMemoryManager physicalMemoryManager{0};

    FIFOPageReplacementPolicy replacementPolicy;

    MemoryManagementUnit mmu(pageTable, physicalMemoryManager, replacementPolicy, PageSize{4096});

    const auto registrationResult = mmu.registerPage(Page{PageId{0}, ProcessId{100}});

    ASSERT_TRUE(registrationResult);

    const auto result = mmu.access
        (MemoryAccess
            {ProcessId{100}, VirtualAddress{0}, MemoryAccessOperation::Read, AccessSequenceNumber{1}}
        );

    EXPECT_FALSE(result.success());

    EXPECT_EQ(mmu.pageFaultCount(), 1U);

    EXPECT_EQ(mmu.pageReplacementCount(), 0U);

    EXPECT_EQ(mmu.dirtyEvictionCount(), 0U);

    EXPECT_TRUE(result.pageFault());

    EXPECT_FALSE(result.pageReplacement());

    EXPECT_FALSE(result.frameId().has_value());

    EXPECT_FALSE(result.physicalAddress().has_value());

    EXPECT_FALSE(result.errorInformation().empty());

    EXPECT_EQ(physicalMemoryManager.capacity(), 0U);

    EXPECT_EQ(physicalMemoryManager.freeFrameCount(), 0U);

    EXPECT_EQ(physicalMemoryManager.allocatedFrameCount(), 0U);

    EXPECT_TRUE(pageTable.empty());

    EXPECT_EQ(replacementPolicy.statistics().replacementCount(), 0U);

    EXPECT_EQ(replacementPolicy.statistics().dirtyEvictionCount(), 0U);
}

} // namespace