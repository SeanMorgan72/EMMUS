#include "emmus/simulation/workload/SequentialWorkload.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace emmus::simulation::workload
{

namespace
{

using memory::access::MemoryAccess;
using memory::access::PageSize;
using memory::identifiers::PageId;
using memory::identifiers::ProcessId;


std::vector<MemoryAccess> drain(
    SequentialWorkload& workload
)
{
    std::vector<MemoryAccess> accesses;

    while (workload.hasNext())
    {
        accesses.push_back(workload.nextAccess());
    }

    return accesses;
}


std::vector<std::uint64_t> virtualAddresses(
    SequentialWorkload& workload
)
{
    std::vector<std::uint64_t> addresses;

    while (workload.hasNext())
    {
        addresses.push_back(
            workload.nextAccess().virtualAddress().value()
        );
    }

    return addresses;
}

} // namespace


TEST(
    SequentialWorkloadTest,
    GeneratesRequestedNumberOfAccesses
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        4U,
        PageSize{4096U},
        8U
    );

    const auto accesses = drain(workload);

    EXPECT_EQ(accesses.size(), 8U);
    EXPECT_FALSE(workload.hasNext());
}


TEST(
    SequentialWorkloadTest,
    GeneratesAddressesInSequentialOrder
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        4U,
        PageSize{4096U},
        4U
    );

    const auto accesses = drain(workload);

    ASSERT_EQ(accesses.size(), 4U);

    EXPECT_EQ(accesses[0].virtualAddress().value(), 0U);
    EXPECT_EQ(accesses[1].virtualAddress().value(), 4096U);
    EXPECT_EQ(accesses[2].virtualAddress().value(), 8192U);
    EXPECT_EQ(accesses[3].virtualAddress().value(), 12288U);
}


TEST(
    SequentialWorkloadTest,
    RepeatsEntireAddressSpacePatternByDefault
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        3U,
        PageSize{4096U},
        8U
    );

    const auto addresses = virtualAddresses(workload);

    const std::vector<std::uint64_t> expected{
        0U,
        4096U,
        8192U,
        0U,
        4096U,
        8192U,
        0U,
        4096U
    };

    EXPECT_EQ(addresses, expected);
}


TEST(
    SequentialWorkloadTest,
    SupportsNonZeroStartingPage
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        8U,
        PageSize{4096U},
        4U,
        PageId{3U},
        4U
    );

    const auto addresses = virtualAddresses(workload);

    const std::vector<std::uint64_t> expected{
        12288U,
        16384U,
        20480U,
        24576U
    };

    EXPECT_EQ(addresses, expected);
}


TEST(
    SequentialWorkloadTest,
    RepeatsConfiguredPatternLength
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        8U,
        PageSize{4096U},
        10U,
        PageId{2U},
        3U
    );

    const auto addresses = virtualAddresses(workload);

    const std::vector<std::uint64_t> expected{
        8192U,
        12288U,
        16384U,
        8192U,
        12288U,
        16384U,
        8192U,
        12288U,
        16384U,
        8192U
    };

    EXPECT_EQ(addresses, expected);
}


TEST(
    SequentialWorkloadTest,
    SupportsAccessCountSmallerThanPatternLength
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        10U,
        PageSize{4096U},
        2U,
        PageId{4U},
        5U
    );

    const auto addresses = virtualAddresses(workload);

    const std::vector<std::uint64_t> expected{
        16384U,
        20480U
    };

    EXPECT_EQ(addresses, expected);
}


TEST(
    SequentialWorkloadTest,
    SupportsAccessCountLargerThanPatternLength
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        10U,
        PageSize{4096U},
        7U,
        PageId{1U},
        2U
    );

    const auto addresses = virtualAddresses(workload);

    const std::vector<std::uint64_t> expected{
        4096U,
        8192U,
        4096U,
        8192U,
        4096U,
        8192U,
        4096U
    };

    EXPECT_EQ(addresses, expected);
}


TEST(
    SequentialWorkloadTest,
    GeneratesAllAccessesForConfiguredProcess
)
{
    SequentialWorkload workload(
        ProcessId{7U},
        4U,
        PageSize{4096U},
        8U,
        PageId{1U},
        2U
    );

    const auto accesses = drain(workload);

    ASSERT_EQ(accesses.size(), 8U);

    for (const auto& access : accesses)
    {
        EXPECT_EQ(access.processId(), ProcessId{7U});
    }
}


TEST(
    SequentialWorkloadTest,
    GeneratesReadAccesses
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        4U,
        PageSize{4096U},
        4U
    );

    const auto accesses = drain(workload);

    ASSERT_EQ(accesses.size(), 4U);

    for (const auto& access : accesses)
    {
        EXPECT_TRUE(access.isRead());
        EXPECT_FALSE(access.isWrite());
    }
}


TEST(
    SequentialWorkloadTest,
    GeneratesSequentialAccessSequenceNumbers
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        4U,
        PageSize{4096U},
        5U,
        PageId{1U},
        2U
    );

    const auto accesses = drain(workload);

    ASSERT_EQ(accesses.size(), 5U);

    for (std::size_t index = 0U; index < accesses.size(); ++index)
    {
        EXPECT_EQ(
            accesses[index].sequenceNumber().value(),
            static_cast<std::uint64_t>(index)
        );
    }
}


TEST(
    SequentialWorkloadTest,
    ResetRestoresInitialSequence
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        8U,
        PageSize{4096U},
        6U,
        PageId{2U},
        3U
    );

    ASSERT_TRUE(workload.hasNext());

    const auto firstAccess =
        workload.nextAccess();

    static_cast<void>(workload.nextAccess());
    static_cast<void>(workload.nextAccess());

    workload.reset();

    const auto resetFirstAccess =
        workload.nextAccess();

    EXPECT_EQ(firstAccess, resetFirstAccess);
}


TEST(
    SequentialWorkloadTest,
    ResetRestoresCompleteSequence
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        8U,
        PageSize{4096U},
        9U,
        PageId{2U},
        3U
    );

    const auto firstSequence = drain(workload);

    workload.reset();

    const auto secondSequence = drain(workload);

    EXPECT_EQ(firstSequence, secondSequence);
}


TEST(
    SequentialWorkloadTest,
    SameConfigurationProducesIdenticalSequence
)
{
    SequentialWorkload first(
        ProcessId{5U},
        12U,
        PageSize{4096U},
        20U,
        PageId{3U},
        4U
    );

    SequentialWorkload second(
        ProcessId{5U},
        12U,
        PageSize{4096U},
        20U,
        PageId{3U},
        4U
    );

    EXPECT_EQ(
        drain(first),
        drain(second)
    );
}


TEST(
    SequentialWorkloadTest,
    ReportsConfiguredSize
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        8U,
        PageSize{4096U},
        13U,
        PageId{2U},
        3U
    );

    EXPECT_EQ(workload.size(), 13U);
}


TEST(
    SequentialWorkloadTest,
    ThrowsWhenPageCountIsZero
)
{
    EXPECT_THROW(
        SequentialWorkload(
            ProcessId{1U},
            0U,
            PageSize{4096U},
            1U
        ),
        std::invalid_argument
    );
}


TEST(
    SequentialWorkloadTest,
    ThrowsWhenAccessCountIsZero
)
{
    EXPECT_THROW(
        SequentialWorkload(
            ProcessId{1U},
            4U,
            PageSize{4096U},
            0U
        ),
        std::invalid_argument
    );
}


TEST(
    SequentialWorkloadTest,
    ThrowsWhenPatternLengthIsZero
)
{
    EXPECT_THROW(
        SequentialWorkload(
            ProcessId{1U},
            4U,
            PageSize{4096U},
            1U,
            PageId{0U},
            0U
        ),
        std::invalid_argument
    );
}


TEST(
    SequentialWorkloadTest,
    ThrowsWhenStartingPageIsOutsideAddressSpace
)
{
    EXPECT_THROW(
        SequentialWorkload(
            ProcessId{1U},
            4U,
            PageSize{4096U},
            1U,
            PageId{4U},
            1U
        ),
        std::invalid_argument
    );
}


TEST(
    SequentialWorkloadTest,
    ThrowsWhenPatternExtendsBeyondAddressSpace
)
{
    EXPECT_THROW(
        SequentialWorkload(
            ProcessId{1U},
            4U,
            PageSize{4096U},
            1U,
            PageId{2U},
            3U
        ),
        std::invalid_argument
    );
}


TEST(
    SequentialWorkloadTest,
    ThrowsWhenVirtualAddressWouldOverflow
)
{
    constexpr auto maximumPageCount =
        static_cast<std::size_t>(
            std::numeric_limits<std::uint64_t>::max()
        );

    /*
     * A one-page pattern beginning at UINT64_MAX with a page size greater
     * than one would require an overflowing virtual address.
     */
    EXPECT_THROW(
        SequentialWorkload(
            ProcessId{1U},
            maximumPageCount,
            PageSize{2U},
            1U,
            PageId{
                std::numeric_limits<std::uint64_t>::max()
            },
            1U
        ),
        std::invalid_argument
    );
}


TEST(
    SequentialWorkloadTest,
    ThrowsWhenNextAccessIsRequestedAfterExhaustion
)
{
    SequentialWorkload workload(
        ProcessId{1U},
        2U,
        PageSize{4096U},
        1U
    );

    static_cast<void>(workload.nextAccess());

    EXPECT_FALSE(workload.hasNext());

    EXPECT_THROW(
        static_cast<void>(workload.nextAccess()),
        std::out_of_range
    );
}

} // namespace emmus::simulation::workload