#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/simulation/workload/IWorkload.hpp"
#include "emmus/simulation/workload/MultiProcessWorkload.hpp"
#include "emmus/simulation/workload/RandomWorkload.hpp"
#include "emmus/simulation/workload/SequentialWorkload.hpp"

using emmus::memory::access::PageSize;
using emmus::memory::identifiers::ProcessId;
using emmus::simulation::workload::IWorkload;
using emmus::simulation::workload::MultiProcessWorkload;
using emmus::simulation::workload::RandomWorkload;
using emmus::simulation::workload::SequentialWorkload;

namespace {

std::vector<std::unique_ptr<IWorkload>> makeRandomWorkloads(
    std::size_t processCount,
    std::size_t pageCountPerProcess,
    PageSize pageSize,
    std::size_t accessesPerProcess,
    std::uint64_t seed)
{
    std::vector<std::unique_ptr<IWorkload>> workloads;
    workloads.reserve(processCount);

    for (std::size_t processIndex = 0;
         processIndex < processCount;
         ++processIndex)
    {
        const auto processId =
            ProcessId{
                static_cast<std::uint64_t>(processIndex + 1U)};

        workloads.push_back(
            std::make_unique<RandomWorkload>(
                processId,
                pageCountPerProcess,
                pageSize,
                accessesPerProcess,
                seed +
                    static_cast<std::uint64_t>(processIndex)));
    }

    return workloads;
}

} // namespace

TEST(MultiProcessWorkloadTest, GeneratesRequestedNumberOfAccesses)
{
    auto workloads = makeRandomWorkloads(
        3,
        4,
        PageSize{4096},
        10,
        12345);

    MultiProcessWorkload workload(std::move(workloads));

    std::size_t count = 0;

    while (workload.hasNext())
    {
        (void)workload.nextAccess();
        ++count;
    }

    EXPECT_EQ(count, 30U);
    EXPECT_EQ(workload.size(), 30U);
}

TEST(MultiProcessWorkloadTest, GeneratesAccessesForMultipleProcesses)
{
    auto workloads = makeRandomWorkloads(
        3,
        4,
        PageSize{4096},
        10,
        12345);

    MultiProcessWorkload workload(std::move(workloads));

    std::set<std::uint64_t> processIds;

    while (workload.hasNext())
    {
        const auto access = workload.nextAccess();

        processIds.insert(access.processId().value());
    }

    EXPECT_EQ(processIds.size(), 3U);
    EXPECT_TRUE(processIds.contains(1U));
    EXPECT_TRUE(processIds.contains(2U));
    EXPECT_TRUE(processIds.contains(3U));
}

TEST(MultiProcessWorkloadTest, SameSeedProducesSameWorkload)
{
    auto firstWorkloads = makeRandomWorkloads(
        3,
        4,
        PageSize{4096},
        10,
        12345);

    auto secondWorkloads = makeRandomWorkloads(
        3,
        4,
        PageSize{4096},
        10,
        12345);

    MultiProcessWorkload first(std::move(firstWorkloads));
    MultiProcessWorkload second(std::move(secondWorkloads));

    ASSERT_EQ(first.size(), second.size());

    while (first.hasNext() && second.hasNext())
    {
        EXPECT_EQ(
            first.nextAccess(),
            second.nextAccess());
    }

    EXPECT_FALSE(first.hasNext());
    EXPECT_FALSE(second.hasNext());
}

TEST(MultiProcessWorkloadTest, ResetRestartsAllChildWorkloads)
{
    auto workloads = makeRandomWorkloads(
        3,
        4,
        PageSize{4096},
        10,
        12345);

    MultiProcessWorkload workload(std::move(workloads));

    std::vector<emmus::memory::access::MemoryAccess> firstRun;

    while (workload.hasNext())
    {
        firstRun.push_back(workload.nextAccess());
    }

    ASSERT_EQ(firstRun.size(), 30U);

    workload.reset();

    std::vector<emmus::memory::access::MemoryAccess> secondRun;

    while (workload.hasNext())
    {
        secondRun.push_back(workload.nextAccess());
    }

    ASSERT_EQ(secondRun.size(), firstRun.size());

    for (std::size_t i = 0; i < firstRun.size(); ++i)
    {
        EXPECT_EQ(firstRun[i], secondRun[i]);
    }
}

TEST(MultiProcessWorkloadTest, EmptyWorkloadHasNoNextAccess)
{
    MultiProcessWorkload workload(
        std::vector<std::unique_ptr<IWorkload>>{});

    EXPECT_EQ(workload.size(), 0U);
    EXPECT_FALSE(workload.hasNext());
}

TEST(MultiProcessWorkloadTest, RejectsNullChildWorkload)
{
    std::vector<std::unique_ptr<IWorkload>> workloads;
    workloads.push_back(nullptr);

    EXPECT_THROW(
        MultiProcessWorkload(std::move(workloads)),
        std::invalid_argument);
}

TEST(MultiProcessWorkloadTest, SupportsDifferentWorkloadTypes)
{
    std::vector<std::unique_ptr<IWorkload>> workloads;

    workloads.push_back(
        std::make_unique<SequentialWorkload>(
            ProcessId{1},
            4,
            PageSize{4096},
            5));

    workloads.push_back(
        std::make_unique<RandomWorkload>(
            ProcessId{2},
            4,
            PageSize{4096},
            5,
            12345));

    MultiProcessWorkload workload(std::move(workloads));

    EXPECT_EQ(workload.size(), 10U);

    std::set<std::uint64_t> processIds;

    while (workload.hasNext())
    {
        const auto access = workload.nextAccess();

        processIds.insert(access.processId().value());
    }

    EXPECT_EQ(processIds.size(), 2U);
    EXPECT_TRUE(processIds.contains(1U));
    EXPECT_TRUE(processIds.contains(2U));
}