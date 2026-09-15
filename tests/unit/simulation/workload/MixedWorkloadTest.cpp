#include "emmus/simulation/workload/MixedWorkload.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/simulation/workload/RandomWorkload.hpp"
#include "emmus/simulation/workload/SequentialWorkload.hpp"

namespace emmus::simulation::workload {

namespace {

using memory::access::MemoryAccess;
using memory::access::MemoryAccessOperation;
using memory::access::PageSize;
using memory::identifiers::ProcessId;
using memory::access::VirtualAddress;

class MixedWorkloadTest : public ::testing::Test {
protected:
    static constexpr std::size_t PageCount = 16;
    static constexpr std::uint64_t PageSizeBytes = 4096;
    static constexpr ProcessId Process{1};
};

} // namespace

TEST_F(MixedWorkloadTest, AggregatesSegmentSizes)
{
    std::vector<std::unique_ptr<IWorkload>> segments;

    segments.push_back(
        std::make_unique<SequentialWorkload>(
            Process,
            PageCount,
            PageSize{PageSizeBytes},
            10));

    segments.push_back(
        std::make_unique<SequentialWorkload>(
            Process,
            PageCount,
            PageSize{PageSizeBytes},
            20));

    MixedWorkload workload(std::move(segments));

    EXPECT_EQ(workload.size(), 30U);
}

TEST_F(MixedWorkloadTest, EmitsFirstSegmentBeforeSecond)
{
    std::vector<std::unique_ptr<IWorkload>> segments;

    segments.push_back(
        std::make_unique<SequentialWorkload>(
            Process,
            PageCount,
            PageSize{PageSizeBytes},
            2));

    segments.push_back(
        std::make_unique<RandomWorkload>(
            Process,
            PageCount,
            PageSize{PageSizeBytes},
            2,
            12345));

    MixedWorkload workload(std::move(segments));

    ASSERT_TRUE(workload.hasNext());

    const auto first = workload.nextAccess();
    const auto second = workload.nextAccess();
    const auto third = workload.nextAccess();

    EXPECT_EQ(first.sequenceNumber().value(), 0U);
    EXPECT_EQ(second.sequenceNumber().value(), 1U);

    EXPECT_EQ(third.sequenceNumber().value(), 0U);
}

TEST_F(MixedWorkloadTest, ResetProducesSameSequence)
{
    std::vector<std::unique_ptr<IWorkload>> segments;

    segments.push_back(
        std::make_unique<SequentialWorkload>(
            Process,
            PageCount,
            PageSize{PageSizeBytes},
            4));

    segments.push_back(
        std::make_unique<RandomWorkload>(
            Process,
            PageCount,
            PageSize{PageSizeBytes},
            4,
            98765));

    MixedWorkload workload(std::move(segments));

    std::vector<MemoryAccess> firstRun;

    while (workload.hasNext()) {
        firstRun.push_back(workload.nextAccess());
    }

    workload.reset();

    std::vector<MemoryAccess> secondRun;

    while (workload.hasNext()) {
        secondRun.push_back(workload.nextAccess());
    }

    ASSERT_EQ(firstRun.size(), secondRun.size());

    for (std::size_t index = 0; index < firstRun.size(); ++index) {
        EXPECT_EQ(firstRun[index], secondRun[index]);
    }
}

TEST_F(MixedWorkloadTest, RejectsNullSegment)
{
    std::vector<std::unique_ptr<IWorkload>> segments;
    segments.push_back(nullptr);

    EXPECT_THROW(
        MixedWorkload(std::move(segments)),
        std::invalid_argument);
}

TEST_F(MixedWorkloadTest, EmptyWorkloadIsValid)
{
    MixedWorkload workload({});

    EXPECT_EQ(workload.size(), 0U);
    EXPECT_FALSE(workload.hasNext());
}

TEST_F(MixedWorkloadTest, ExhaustionThrows)
{
    std::vector<std::unique_ptr<IWorkload>> segments;

    segments.push_back(
        std::make_unique<SequentialWorkload>(
            Process,
            PageCount,
            PageSize{PageSizeBytes},
            1));

    MixedWorkload workload(std::move(segments));

    ASSERT_TRUE(workload.hasNext());
    static_cast<void>(workload.nextAccess());

    EXPECT_FALSE(workload.hasNext());

    EXPECT_THROW(
        static_cast<void>(workload.nextAccess()),
        std::out_of_range);
}

} // namespace emmus::simulation::workload