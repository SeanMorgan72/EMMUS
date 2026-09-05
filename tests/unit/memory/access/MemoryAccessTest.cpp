#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace {

using emmus::memory::access::AccessSequenceNumber;
using emmus::memory::access::MemoryAccess;
using emmus::memory::access::MemoryAccessOperation;
using emmus::memory::access::PageOffset;
using emmus::memory::access::PageSize;
using emmus::memory::access::PhysicalAddress;
using emmus::memory::access::VirtualAddress;

using emmus::memory::identifiers::FrameId;
using emmus::memory::identifiers::PageId;
using emmus::memory::identifiers::ProcessId;

// -----------------------------------------------------------------------------
// Construction and accessors
// -----------------------------------------------------------------------------

TEST(MemoryAccessTest, ConstructsReadAccessWithExpectedValues)
{
    constexpr ProcessId processId{100};
    constexpr VirtualAddress virtualAddress{0x1234};
    constexpr AccessSequenceNumber sequenceNumber{42};

    const MemoryAccess access{
        processId,
        virtualAddress,
        MemoryAccessOperation::Read,
        sequenceNumber
    };

    EXPECT_EQ(access.processId(), processId);
    EXPECT_EQ(access.virtualAddress(), virtualAddress);
    EXPECT_EQ(access.operation(), MemoryAccessOperation::Read);
    EXPECT_EQ(access.sequenceNumber(), sequenceNumber);
}

TEST(MemoryAccessTest, ConstructsWriteAccessWithExpectedValues)
{
    constexpr ProcessId processId{200};
    constexpr VirtualAddress virtualAddress{0x5678};
    constexpr AccessSequenceNumber sequenceNumber{99};

    const MemoryAccess access{
        processId,
        virtualAddress,
        MemoryAccessOperation::Write,
        sequenceNumber
    };

    EXPECT_EQ(access.processId(), processId);
    EXPECT_EQ(access.virtualAddress(), virtualAddress);
    EXPECT_EQ(access.operation(), MemoryAccessOperation::Write);
    EXPECT_EQ(access.sequenceNumber(), sequenceNumber);
}

// -----------------------------------------------------------------------------
// Read/write classification
// -----------------------------------------------------------------------------

TEST(MemoryAccessTest, ReadAccessIsIdentifiedAsRead)
{
    const MemoryAccess access{
        ProcessId{100},
        VirtualAddress{0x1000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{1}
    };

    EXPECT_TRUE(access.isRead());
    EXPECT_FALSE(access.isWrite());
}

TEST(MemoryAccessTest, WriteAccessIsIdentifiedAsWrite)
{
    const MemoryAccess access{
        ProcessId{100},
        VirtualAddress{0x1000},
        MemoryAccessOperation::Write,
        AccessSequenceNumber{1}
    };

    EXPECT_TRUE(access.isWrite());
    EXPECT_FALSE(access.isRead());
}

// -----------------------------------------------------------------------------
// Equality
// -----------------------------------------------------------------------------

TEST(MemoryAccessTest, EqualAccessesCompareEqual)
{
    const MemoryAccess first{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    const MemoryAccess second{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    EXPECT_EQ(first, second);
}

TEST(MemoryAccessTest, DifferentProcessIdsCompareNotEqual)
{
    const MemoryAccess first{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    const MemoryAccess second{
        ProcessId{200},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    EXPECT_NE(first, second);
}

TEST(MemoryAccessTest, DifferentVirtualAddressesCompareNotEqual)
{
    const MemoryAccess first{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    const MemoryAccess second{
        ProcessId{100},
        VirtualAddress{0x3000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    EXPECT_NE(first, second);
}

TEST(MemoryAccessTest, DifferentOperationsCompareNotEqual)
{
    const MemoryAccess first{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    const MemoryAccess second{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Write,
        AccessSequenceNumber{10}
    };

    EXPECT_NE(first, second);
}

TEST(MemoryAccessTest, DifferentSequenceNumbersCompareNotEqual)
{
    const MemoryAccess first{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    const MemoryAccess second{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{11}
    };

    EXPECT_NE(first, second);
}

TEST(MemoryAccessTest, InequalityOperatorWorksForDifferentAccesses)
{
    const MemoryAccess first{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{10}
    };

    const MemoryAccess second{
        ProcessId{100},
        VirtualAddress{0x2000},
        MemoryAccessOperation::Write,
        AccessSequenceNumber{10}
    };

    EXPECT_TRUE(first != second);
}

// -----------------------------------------------------------------------------
// constexpr behavior
// -----------------------------------------------------------------------------

TEST(MemoryAccessTest, SupportsConstexprConstructionAndAccess)
{
    constexpr MemoryAccess access{
        ProcessId{123},
        VirtualAddress{0xABCDEF},
        MemoryAccessOperation::Write,
        AccessSequenceNumber{77}
    };

    static_assert(access.processId() == ProcessId{123});
    static_assert(access.virtualAddress() == VirtualAddress{0xABCDEF});
    static_assert(
        access.operation() == MemoryAccessOperation::Write);
    static_assert(
        access.sequenceNumber() == AccessSequenceNumber{77});
    static_assert(access.isWrite());
    static_assert(!access.isRead());

    EXPECT_EQ(access.processId(), ProcessId{123});
    EXPECT_EQ(access.virtualAddress(), VirtualAddress{0xABCDEF});
    EXPECT_EQ(access.operation(), MemoryAccessOperation::Write);
    EXPECT_EQ(access.sequenceNumber(), AccessSequenceNumber{77});
}

// -----------------------------------------------------------------------------
// Type-safety
// -----------------------------------------------------------------------------

TEST(MemoryAccessTest, MemoryAccessOperationIsStronglyTyped)
{
    static_assert(
        !std::is_convertible_v<MemoryAccessOperation, int>);

    static_assert(
        !std::is_convertible_v<int, MemoryAccessOperation>);

    SUCCEED();
}

TEST(MemoryAccessTest, MemoryObjectIdentifiersRemainDistinctTypes)
{
    static_assert(!std::is_same_v<ProcessId, PageId>);
    static_assert(!std::is_same_v<ProcessId, FrameId>);
    static_assert(!std::is_same_v<PageId, FrameId>);

    static_assert(!std::is_convertible_v<ProcessId, PageId>);
    static_assert(!std::is_convertible_v<ProcessId, FrameId>);
    static_assert(!std::is_convertible_v<PageId, ProcessId>);
    static_assert(!std::is_convertible_v<FrameId, ProcessId>);

    SUCCEED();
}

// -----------------------------------------------------------------------------
// Boundary/value preservation
// -----------------------------------------------------------------------------

TEST(MemoryAccessTest, PreservesZeroSequenceNumber)
{
    const MemoryAccess access{
        ProcessId{100},
        VirtualAddress{0},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{0}
    };

    EXPECT_EQ(access.sequenceNumber(), AccessSequenceNumber{0});
}

TEST(MemoryAccessTest, PreservesLargeAddressAndSequenceValues)
{
    constexpr std::uint64_t maxValue =
        UINT64_MAX;

    const MemoryAccess access{
        ProcessId{100},
        VirtualAddress{maxValue},
        MemoryAccessOperation::Read,
        AccessSequenceNumber{maxValue}
    };

    EXPECT_EQ(access.virtualAddress(), VirtualAddress{maxValue});
    EXPECT_EQ(
        access.sequenceNumber(),
        AccessSequenceNumber{maxValue});
}

} // namespace