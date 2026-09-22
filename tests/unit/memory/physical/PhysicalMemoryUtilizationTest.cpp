#include <gtest/gtest.h>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/physical/PhysicalMemoryUtilization.hpp"

namespace emmus::test
{

class PhysicalMemoryUtilizationTest : public ::testing::Test
{
protected:
    using PageId = emmus::memory::identifiers::PageId;
    using FrameId = emmus::memory::identifiers::FrameId;
    using Manager = emmus::memory::physical::PhysicalMemoryManager;
    using Utilization = emmus::memory::physical::PhysicalMemoryUtilization;
};

TEST_F(PhysicalMemoryUtilizationTest, EmptyMemoryReportsZeroUtilization)
{
    Manager manager{4};

    const auto snapshot = manager.snapshotUtilization();

    EXPECT_EQ(snapshot.totalFrames, 4U);
    EXPECT_EQ(snapshot.allocatedFrames, 0U);
    EXPECT_EQ(snapshot.freeFrames, 4U);
    EXPECT_DOUBLE_EQ(snapshot.utilizationRatio, 0.0);
    EXPECT_DOUBLE_EQ(snapshot.utilizationPercent, 0.0);
    EXPECT_TRUE(snapshot.frames.empty() == false);
    EXPECT_EQ(snapshot.frames.size(), 4U);

    for (const auto& frame : snapshot.frames)
    {
        EXPECT_EQ(frame.state, emmus::memory::physical::FrameState::Free);
        EXPECT_FALSE(frame.isAllocated);
        EXPECT_FALSE(frame.isOccupied);
        EXPECT_FALSE(frame.pageId.has_value());
    }
}

TEST_F(PhysicalMemoryUtilizationTest, PartiallyUtilizedMemoryTracksAllocatedFrames)
{
    Manager manager{4};

    const auto first = manager.allocateFrame(PageId{10});
    const auto second = manager.allocateFrame(PageId{11});

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());

    const auto snapshot = manager.snapshotUtilization();

    EXPECT_EQ(snapshot.totalFrames, 4U);
    EXPECT_EQ(snapshot.allocatedFrames, 2U);
    EXPECT_EQ(snapshot.freeFrames, 2U);
    EXPECT_DOUBLE_EQ(snapshot.utilizationRatio, 0.5);
    EXPECT_DOUBLE_EQ(snapshot.utilizationPercent, 50.0);

    EXPECT_EQ(
        snapshot.frames[first.value().value()].state,
        emmus::memory::physical::FrameState::Allocated);

    EXPECT_EQ(
        snapshot.frames[second.value().value()].state,
        emmus::memory::physical::FrameState::Allocated);

    EXPECT_TRUE(snapshot.frames[first.value().value()].isAllocated);
    EXPECT_TRUE(snapshot.frames[second.value().value()].isAllocated);

    EXPECT_TRUE(snapshot.frames[first.value().value()].pageId.has_value());
    EXPECT_TRUE(snapshot.frames[second.value().value()].pageId.has_value());
}

TEST_F(PhysicalMemoryUtilizationTest, FullyUtilizedMemoryReportsOneHundredPercent)
{
    Manager manager{3};

    ASSERT_TRUE(manager.allocateFrame(PageId{1}).has_value());
    ASSERT_TRUE(manager.allocateFrame(PageId{2}).has_value());
    ASSERT_TRUE(manager.allocateFrame(PageId{3}).has_value());

    const auto snapshot = manager.snapshotUtilization();

    EXPECT_EQ(snapshot.totalFrames, 3U);
    EXPECT_EQ(snapshot.allocatedFrames, 3U);
    EXPECT_EQ(snapshot.freeFrames, 0U);
    EXPECT_DOUBLE_EQ(snapshot.utilizationRatio, 1.0);
    EXPECT_DOUBLE_EQ(snapshot.utilizationPercent, 100.0);
    EXPECT_TRUE(snapshot.isFullyUtilized());
    EXPECT_FALSE(snapshot.isEmpty());

    for (const auto& frame : snapshot.frames)
    {
        EXPECT_EQ(frame.state, emmus::memory::physical::FrameState::Allocated);
        EXPECT_TRUE(frame.isAllocated);
        EXPECT_TRUE(frame.isOccupied);
    }
}

TEST_F(PhysicalMemoryUtilizationTest, AllocationAndReleaseUpdateTheSnapshot)
{
    Manager manager{2};

    const auto first = manager.allocateFrame(PageId{100});
    ASSERT_TRUE(first.has_value());

    auto snapshot = manager.snapshotUtilization();
    EXPECT_EQ(snapshot.allocatedFrames, 1U);
    EXPECT_EQ(snapshot.freeFrames, 1U);

    EXPECT_TRUE(manager.releaseFrame(first.value()));

    snapshot = manager.snapshotUtilization();
    EXPECT_EQ(snapshot.allocatedFrames, 0U);
    EXPECT_EQ(snapshot.freeFrames, 2U);
    EXPECT_DOUBLE_EQ(snapshot.utilizationRatio, 0.0);
    EXPECT_TRUE(snapshot.frames[first.value().value()].state ==
                emmus::memory::physical::FrameState::Free);
}

TEST_F(PhysicalMemoryUtilizationTest, PageReplacementKeepsSnapshotConsistent)
{
    Manager manager{2};

    const auto first = manager.allocateFrame(PageId{1});
    const auto second = manager.allocateFrame(PageId{2});

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());

    const auto replacementFrame = first.value();
    const auto replacer = manager.frameForPage(PageId{2});
    ASSERT_TRUE(replacer.has_value());

    EXPECT_EQ(manager.frame(replacementFrame)->mappedPage().value(), PageId{1});
    EXPECT_EQ(manager.frame(replacer.value())->mappedPage().value(), PageId{2});

    EXPECT_EQ(manager.snapshotUtilization().allocatedFrames, 2U);
    EXPECT_EQ(manager.snapshotUtilization().freeFrames, 0U);
}

TEST_F(PhysicalMemoryUtilizationTest, WorkloadChangeUpdatesPhysicalMemoryUtilization)
{
    Manager manager{4};

    for (std::size_t i = 0; i < 4; ++i)
    {
        ASSERT_TRUE(manager.allocateFrame(PageId{static_cast<PageId::ValueType>(i + 1)}).has_value());
    }

    auto snapshot = manager.snapshotUtilization();
    EXPECT_DOUBLE_EQ(snapshot.utilizationPercent, 100.0);

    EXPECT_TRUE(manager.releaseFrame(FrameId{2}));

    snapshot = manager.snapshotUtilization();
    EXPECT_EQ(snapshot.allocatedFrames, 3U);
    EXPECT_EQ(snapshot.freeFrames, 1U);
    EXPECT_DOUBLE_EQ(snapshot.utilizationPercent, 75.0);
    EXPECT_EQ(snapshot.frames[2].state, emmus::memory::physical::FrameState::Free);
}

} // namespace emmus::test
