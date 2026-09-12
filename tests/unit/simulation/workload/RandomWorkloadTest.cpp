#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/simulation/workload/RandomWorkload.hpp"

using emmus::memory::access::MemoryAccess;
using emmus::memory::access::PageSize;
using emmus::memory::identifiers::ProcessId;
using emmus::simulation::workload::RandomWorkload;

namespace {

RandomWorkload makeWorkload(
    std::size_t pageCount = 16,
    std::size_t accessCount = 100,
    std::uint64_t seed = 12345)
{
    return RandomWorkload(
        ProcessId{1},
        pageCount,
        PageSize{4096},
        accessCount,
        seed);
}

} // namespace

TEST(RandomWorkloadTest, GeneratesRequestedNumberOfAccesses)
{
    auto workload = makeWorkload();

    std::size_t count = 0;

    while (workload.hasNext())
    {
        (void)workload.nextAccess();
        ++count;
    }

    EXPECT_EQ(count, 100U);
    EXPECT_EQ(workload.size(), 100U);
}

TEST(RandomWorkloadTest, SameSeedProducesSameWorkload)
{
    auto first = makeWorkload();
    auto second = makeWorkload();

    while (first.hasNext() && second.hasNext())
    {
        EXPECT_EQ(
            first.nextAccess(),
            second.nextAccess());
    }

    EXPECT_FALSE(first.hasNext());
    EXPECT_FALSE(second.hasNext());
}

TEST(RandomWorkloadTest, ResetProducesSameSequence)
{
    auto workload = makeWorkload(
        16,
        50,
        12345);

    std::vector<MemoryAccess> firstRun;

    while (workload.hasNext())
    {
        firstRun.push_back(workload.nextAccess());
    }

    workload.reset();

    std::vector<MemoryAccess> secondRun;

    while (workload.hasNext())
    {
        secondRun.push_back(workload.nextAccess());
    }

    EXPECT_EQ(firstRun, secondRun);
}

TEST(RandomWorkloadTest, GeneratesAccessesForConfiguredProcess)
{
    auto workload = RandomWorkload(
        ProcessId{7},
        16,
        PageSize{4096},
        25,
        12345);

    while (workload.hasNext())
    {
        const auto access = workload.nextAccess();

        EXPECT_EQ(
            access.processId(),
            ProcessId{7});
    }
}