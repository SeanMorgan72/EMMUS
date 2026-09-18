#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/application/MemoryAccessExecutor.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/mmu/MemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual_memory/Page.hpp"

namespace emmus::test
{

class SubsystemIntegrationFixture : public ::testing::Test
{
protected:
    using Access = emmus::memory::access::MemoryAccess;
    using AccessOperation = emmus::memory::access::MemoryAccessOperation;
    using FIFOPageReplacementPolicy =
        emmus::algorithms::replacement::FIFOPageReplacementPolicy;
    using MemoryAccessExecutor = emmus::application::MemoryAccessExecutor;
    using MemoryManagementUnit = emmus::memory::mmu::MemoryManagementUnit;
    using Page = emmus::memory::virtual_memory::Page;
    using PageTable = emmus::memory::mmu::PageTable;
    using PageTableIntegration =
        emmus::memory::mmu::PageTablePhysicalMemoryIntegration;
    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;
    using ProcessId = emmus::memory::identifiers::ProcessId;
    using PageId = emmus::memory::identifiers::PageId;
    using FrameId = emmus::memory::identifiers::FrameId;
    using PageSize = emmus::memory::access::PageSize;
    using VirtualAddress = emmus::memory::access::VirtualAddress;
    using AccessSequenceNumber =
        emmus::memory::access::AccessSequenceNumber;

    static constexpr ProcessId kProcessA{10};
    static constexpr ProcessId kProcessB{20};

    static constexpr PageId kPage0{0};
    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};
    static constexpr PageId kPage3{3};
    static constexpr PageId kPage9{9};

    static constexpr PageSize kPageSize{4096};

    PageTable pageTable_;
    PhysicalMemoryManager physicalMemory_{2};
    FIFOPageReplacementPolicy replacementPolicy_;
    MemoryManagementUnit mmu_{
        pageTable_,
        physicalMemory_,
        replacementPolicy_,
        kPageSize
    };

    static Access makeAccess(
        ProcessId processId,
        PageId pageId,
        AccessOperation operation,
        std::uint64_t sequence,
        std::uint64_t offset = 0U)
    {
        const std::uint64_t addressValue =
            static_cast<std::uint64_t>(pageId.value()) * kPageSize.value() +
            offset;

        return Access{
            processId,
            VirtualAddress{addressValue},
            operation,
            AccessSequenceNumber{sequence}
        };
    }

    void registerPage(
        PageId pageId,
        ProcessId processId = kProcessA)
    {
        ASSERT_TRUE(
            mmu_.registerPage(
                Page{pageId, processId}
            )
        );
    }
};

TEST_F(
    SubsystemIntegrationFixture,
    PageTableAndPhysicalMemoryRemainConsistentDuringMultiPageLoad
)
{
    PageTableIntegration integration{
        pageTable_,
        physicalMemory_
    };

    const auto frame0 = integration.mapPage(kPage0);
    const auto frame1 = integration.mapPage(kPage1);

    ASSERT_TRUE(frame0.has_value());
    ASSERT_TRUE(frame1.has_value());

    EXPECT_EQ(pageTable_.lookup(kPage0), frame0);
    EXPECT_EQ(pageTable_.lookup(kPage1), frame1);
    EXPECT_EQ(physicalMemory_.frameForPage(kPage0), frame0);
    EXPECT_EQ(physicalMemory_.frameForPage(kPage1), frame1);

    EXPECT_TRUE(integration.isMappingConsistent(kPage0));
    EXPECT_TRUE(integration.isMappingConsistent(kPage1));
    EXPECT_TRUE(integration.isConsistent());
    EXPECT_EQ(physicalMemory_.allocatedFrameCount(), 2U);
    EXPECT_EQ(physicalMemory_.freeFrameCount(), 0U);
}

TEST_F(
    SubsystemIntegrationFixture,
    MMUPageFaultLoadsPageAndPublishesStructuredAccessResult
)
{
    registerPage(kPage0);

    const auto result =
        mmu_.access(makeAccess(kProcessA, kPage0, AccessOperation::Read, 1U));

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());

    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), FrameId{0});

    ASSERT_TRUE(result.physicalAddress().has_value());
    EXPECT_EQ(
        result.physicalAddress().value(),
        emmus::memory::access::PhysicalAddress{0U}
    );

    EXPECT_EQ(mmu_.pageFaultCount(), 1U);
    EXPECT_EQ(mmu_.registeredPageCount(), 1U);

    const Page* page = mmu_.page(kPage0);
    ASSERT_NE(page, nullptr);
    EXPECT_TRUE(page->isResident());
    EXPECT_TRUE(page->isReferenced());
    EXPECT_EQ(page->mappedFrame(), result.frameId());
    EXPECT_EQ(pageTable_.lookup(kPage0), result.frameId());
}

TEST_F(
    SubsystemIntegrationFixture,
    MemoryAccessExecutorPreservesAccessOrderAcrossFaultAndHitSequence
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    const std::vector<Access> accesses = {
        makeAccess(kProcessA, kPage0, AccessOperation::Read, 1U),
        makeAccess(kProcessA, kPage0, AccessOperation::Read, 2U),
        makeAccess(kProcessA, kPage2, AccessOperation::Write, 3U),
    };

    MemoryAccessExecutor executor{mmu_};
    const auto result = executor.execute(accesses);

    ASSERT_EQ(result.size(), 3U);
    EXPECT_TRUE(result.records()[0].result.pageFault());
    EXPECT_FALSE(result.records()[1].result.pageFault());
    EXPECT_TRUE(result.records()[2].result.pageFault());
    EXPECT_EQ(result.statistics().memoryAccessCount(), 3U);
    EXPECT_EQ(result.statistics().pageFaultCount(), 2U);
    EXPECT_TRUE(result.records()[0].result.success());
    EXPECT_TRUE(result.records()[1].result.success());
    EXPECT_TRUE(result.records()[2].result.success());
}

TEST_F(
    SubsystemIntegrationFixture,
    MMUReplacementPathEvictsVictimAndTracksReplacementStatistics
)
{
    registerPage(kPage0);
    registerPage(kPage1);
    registerPage(kPage2);

    EXPECT_TRUE(mmu_.access(makeAccess(kProcessA, kPage0, AccessOperation::Read, 1U)).success());
    EXPECT_TRUE(mmu_.access(makeAccess(kProcessA, kPage1, AccessOperation::Read, 2U)).success());

    const auto result =
        mmu_.access(makeAccess(kProcessA, kPage2, AccessOperation::Read, 3U));

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_EQ(mmu_.pageReplacementCount(), 1U);
    EXPECT_TRUE(mmu_.page(kPage2)->isResident());
    EXPECT_TRUE(pageTable_.lookup(kPage2).has_value());
    EXPECT_EQ(mmu_.pageFaultCount(), 3U);
}

TEST_F(
    SubsystemIntegrationFixture,
    InvalidUnregisteredAccessFailsWithoutMutatingSubsystemState
)
{
    const auto beforePageFaultCount = mmu_.pageFaultCount();
    const auto beforePageCount = mmu_.registeredPageCount();

    const auto result =
        mmu_.access(makeAccess(kProcessA, kPage9, AccessOperation::Read, 99U));

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_EQ(mmu_.pageFaultCount(), beforePageFaultCount);
    EXPECT_EQ(mmu_.registeredPageCount(), beforePageCount);
    EXPECT_TRUE(pageTable_.empty());
    EXPECT_EQ(physicalMemory_.allocatedFrameCount(), 0U);
}

} // namespace emmus::test