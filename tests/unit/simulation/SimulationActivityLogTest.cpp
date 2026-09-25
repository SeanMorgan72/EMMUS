#include <gtest/gtest.h>

#include "emmus/simulation/activity/SimulationActivityLog.hpp"

namespace emmus::simulation::activity
{

TEST(SimulationActivityLogTest, RecordsAccessEventsInSequence)
{
    SimulationActivityLog log;

    log.recordAccess(
        emmus::memory::identifiers::ProcessId{1U},
        emmus::memory::identifiers::PageId{7U},
        std::optional<emmus::memory::identifiers::FrameId>{
            emmus::memory::identifiers::FrameId{2U}},
        emmus::memory::access::MemoryAccessOperation::Read,
        true,
        false,
        false,
        false,
        "resident access");

    log.recordAccess(
        emmus::memory::identifiers::ProcessId{1U},
        emmus::memory::identifiers::PageId{9U},
        std::optional<emmus::memory::identifiers::FrameId>{
            emmus::memory::identifiers::FrameId{3U}},
        emmus::memory::access::MemoryAccessOperation::Write,
        true,
        false,
        false,
        false,
        "write access");

    ASSERT_EQ(log.size(), 2U);
    EXPECT_EQ(log.events()[0].sequence, 0U);
    EXPECT_EQ(log.events()[1].sequence, 1U);
    EXPECT_EQ(log.events()[0].type, SimulationActivityType::Access);
    EXPECT_EQ(log.events()[1].operation.value(),
              emmus::memory::access::MemoryAccessOperation::Write);
}

TEST(SimulationActivityLogTest, RecordsPageFaultAndReplacementEvents)
{
    SimulationActivityLog log;

    log.recordPageFault(
        emmus::memory::identifiers::ProcessId{2U},
        emmus::memory::identifiers::PageId{5U},
        std::optional<emmus::memory::identifiers::FrameId>{
            emmus::memory::identifiers::FrameId{1U}},
        "missing page fault");

    log.recordPageReplacement(
        emmus::memory::identifiers::ProcessId{2U},
        emmus::memory::identifiers::PageId{6U},
        emmus::memory::identifiers::FrameId{1U},
        emmus::memory::identifiers::PageId{5U},
        "evict old page");

    ASSERT_EQ(log.size(), 2U);
    EXPECT_EQ(log.events()[0].type, SimulationActivityType::PageFault);
    EXPECT_EQ(log.events()[1].type, SimulationActivityType::PageReplacement);
    EXPECT_TRUE(log.events()[1].pageReplacement);
    EXPECT_EQ(log.events()[1].victimFrameId.value(),
              emmus::memory::identifiers::FrameId{1U});
    EXPECT_EQ(log.events()[1].victimPageId.value(),
              emmus::memory::identifiers::PageId{5U});
}

TEST(SimulationActivityLogTest, SupportsMultipleProcessesAndRapidSequences)
{
    SimulationActivityLog log;

    for (std::size_t i = 0; i < 20; ++i)
    {
        const auto processId =
            emmus::memory::identifiers::ProcessId{static_cast<std::uint64_t>(i % 3U + 1U)};
        const auto pageId =
            emmus::memory::identifiers::PageId{static_cast<std::uint64_t>(i % 9U + 1U)};

        log.recordStatus(
            processId,
            "status update",
            pageId,
            std::optional<emmus::memory::identifiers::FrameId>{
                emmus::memory::identifiers::FrameId{static_cast<std::uint64_t>(i % 4U)}},
            true);
    }

    EXPECT_EQ(log.size(), 20U);
    EXPECT_EQ(log.events().front().sequence, 0U);
    EXPECT_EQ(log.events().back().sequence, 19U);
    EXPECT_EQ(log.events()[10].processId.value(),
              emmus::memory::identifiers::ProcessId{2U});
}

TEST(SimulationActivityLogTest, ClearDropsAllHistory)
{
    SimulationActivityLog log;
    log.recordStatus(
        emmus::memory::identifiers::ProcessId{5U},
        "initial",
        std::nullopt,
        std::nullopt,
        true);

    log.clear();

    EXPECT_TRUE(log.empty());
    EXPECT_EQ(log.size(), 0U);
    EXPECT_TRUE(log.render().empty());
}

TEST(SimulationActivityLogTest, RenderIncludesTypeAndSummary)
{
    SimulationActivityLog log;
    log.recordDirtyEviction(
        emmus::memory::identifiers::ProcessId{8U},
        emmus::memory::identifiers::PageId{12U},
        emmus::memory::identifiers::FrameId{4U},
        "dirty page evicted");

    const auto rendered = log.render();
    EXPECT_NE(rendered.find("DIRTY_EVICTION"), std::string::npos);
    EXPECT_NE(rendered.find("Dirty eviction"), std::string::npos);
}

} // namespace emmus::simulation::activity
