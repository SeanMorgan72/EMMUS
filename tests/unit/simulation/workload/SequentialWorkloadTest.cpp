#include <cstddef>

#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/simulation/workload/SequentialWorkload.hpp"

using emmus::memory::access::PageSize;
using emmus::memory::access::VirtualAddress;
using emmus::memory::identifiers::ProcessId;
using emmus::simulation::workload::SequentialWorkload;

TEST(SequentialWorkloadTest, GeneratesRequestedNumberOfAccesses)
{
    SequentialWorkload workload(
        ProcessId{1},
        4,
        PageSize{4096},
        8);

    std::size_t count = 0;

    while (workload.hasNext())
    {
        (void)workload.nextAccess();
        ++count;
    }

    EXPECT_EQ(count, 8U);
    EXPECT_FALSE(workload.hasNext());
}

TEST(SequentialWorkloadTest, GeneratesAddressesInSequentialOrder)
{
    SequentialWorkload workload(
        ProcessId{1},
        4,
        PageSize{4096},
        4);

    const auto first = workload.nextAccess();
    const auto second = workload.nextAccess();
    const auto third = workload.nextAccess();

    EXPECT_EQ(first.virtualAddress(), VirtualAddress{0U});
    EXPECT_EQ(second.virtualAddress(), VirtualAddress{4096U});
    EXPECT_EQ(third.virtualAddress(), VirtualAddress{8192U});
}

TEST(SequentialWorkloadTest, ResetRestoresInitialSequence)
{
    SequentialWorkload workload(
        ProcessId{1},
        4,
        PageSize{4096},
        4);

    const auto first = workload.nextAccess();

    (void)workload.nextAccess();
    (void)workload.nextAccess();

    workload.reset();

    const auto resetFirst = workload.nextAccess();

    EXPECT_EQ(first, resetFirst);
}

TEST(SequentialWorkloadTest, GeneratesAccessesForConfiguredProcess)
{
    SequentialWorkload workload(
        ProcessId{7},
        4,
        PageSize{4096},
        8);

    while (workload.hasNext())
    {
        const auto access = workload.nextAccess();

        EXPECT_EQ(
            access.processId(),
            ProcessId{7});
    }
}

TEST(SequentialWorkloadTest, ReportsConfiguredSize)
{
    SequentialWorkload workload(
        ProcessId{1},
        4,
        PageSize{4096},
        8);

    EXPECT_EQ(workload.size(), 8U);
}