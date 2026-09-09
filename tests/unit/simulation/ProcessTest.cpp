#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "emmus/simulation/Process.hpp"

namespace emmus::simulation
{

TEST(ProcessTest, CreatesWithExpectedIdentity)
{
    const memory::identifiers::ProcessId processId{42};

    const Process process{
        processId,
        VirtualAddressSpace{16, 4096}
    };

    EXPECT_EQ(process.processId(), processId);
}

TEST(ProcessTest, CreatesWithExpectedVirtualAddressSpace)
{
    const Process process{
        memory::identifiers::ProcessId{1},
        VirtualAddressSpace{128, 4096}
    };

    EXPECT_EQ(
        process.virtualAddressSpace().pageCount(),
        128
    );

    EXPECT_EQ(
        process.virtualAddressSpace().pageSize(),
        4096
    );

    EXPECT_EQ(
        process.virtualAddressSpace().sizeInBytes(),
        128ULL * 4096ULL
    );
}

TEST(ProcessTest, StartsInCreatedState)
{
    const Process process{
        memory::identifiers::ProcessId{1},
        VirtualAddressSpace{16, 4096}
    };

    EXPECT_EQ(
        process.state(),
        ProcessState::Created
    );
}

TEST(ProcessTest, CanTransitionToTerminatedState)
{
    Process process{
        memory::identifiers::ProcessId{1},
        VirtualAddressSpace{16, 4096}
    };

    process.terminate();

    EXPECT_EQ(
        process.state(),
        ProcessState::Terminated
    );
}

TEST(ProcessTest, RejectsZeroPageCount)
{
    EXPECT_THROW(
        (VirtualAddressSpace{0, 4096}),
        std::invalid_argument
    );
}

TEST(ProcessTest, RejectsZeroPageSize)
{
    EXPECT_THROW(
        (VirtualAddressSpace{16, 0}),
        std::invalid_argument
    );
}

TEST(ProcessTest, RejectsAddressSpaceSizeOverflow)
{
    EXPECT_THROW(
        (VirtualAddressSpace{
            std::numeric_limits<std::uint64_t>::max(),
            2
        }),
        std::invalid_argument
    );
}

} // namespace emmus::simulation
