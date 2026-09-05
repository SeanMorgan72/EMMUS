#include <optional>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccessResult.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace {

using emmus::memory::access::MemoryAccessResult;
using emmus::memory::access::PhysicalAddress;

using emmus::memory::identifiers::FrameId;

// -----------------------------------------------------------------------------
// Successful access
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, SuccessfulAccessStoresExpectedState)
{
    const FrameId frameId{7};
    const PhysicalAddress physicalAddress{0x7000};

    const MemoryAccessResult result{
        true,
        false,
        false,
        frameId,
        physicalAddress,
        false
    };

    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), frameId);
    ASSERT_TRUE(result.physicalAddress().has_value());
    EXPECT_EQ(result.physicalAddress().value(), physicalAddress);
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_TRUE(result.errorInformation().empty());
}

// -----------------------------------------------------------------------------
// Page fault
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, PageFaultStoresExpectedState)
{
    const FrameId frameId{3};
    const PhysicalAddress physicalAddress{0x3004};

    const MemoryAccessResult result{
        true,
        true,
        false,
        frameId,
        physicalAddress,
        false
    };

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), frameId);
    ASSERT_TRUE(result.physicalAddress().has_value());
    EXPECT_EQ(result.physicalAddress().value(), physicalAddress);
    EXPECT_FALSE(result.dirtyEviction());
}

// -----------------------------------------------------------------------------
// Page replacement
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, PageReplacementStoresExpectedState)
{
    const FrameId frameId{2};
    const PhysicalAddress physicalAddress{0x2008};

    const MemoryAccessResult result{
        true,
        true,
        true,
        frameId,
        physicalAddress,
        false
    };

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());
    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), frameId);
    ASSERT_TRUE(result.physicalAddress().has_value());
    EXPECT_EQ(result.physicalAddress().value(), physicalAddress);
    EXPECT_FALSE(result.dirtyEviction());
}

// -----------------------------------------------------------------------------
// Dirty eviction
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, DirtyEvictionStoresExpectedState)
{
    const FrameId frameId{5};
    const PhysicalAddress physicalAddress{0x5000};

    const MemoryAccessResult result{
        true,
        true,
        true,
        frameId,
        physicalAddress,
        true
    };

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());
    EXPECT_TRUE(result.dirtyEviction());
    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), frameId);
    ASSERT_TRUE(result.physicalAddress().has_value());
    EXPECT_EQ(result.physicalAddress().value(), physicalAddress);
}

// -----------------------------------------------------------------------------
// Failed access
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, FailedAccessCanContainErrorInformation)
{
    const std::string errorMessage{
        "Invalid virtual address"
    };

    const MemoryAccessResult result{
        false,
        false,
        false,
        std::nullopt,
        std::nullopt,
        false,
        errorMessage
    };

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
    EXPECT_FALSE(result.dirtyEviction());
    EXPECT_EQ(result.errorInformation(), errorMessage);
}

// -----------------------------------------------------------------------------
// Optional state
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, FrameIdCanBeAbsent)
{
    const MemoryAccessResult result{
        false,
        false,
        false,
        std::nullopt,
        PhysicalAddress{0x1000},
        false,
        "Frame allocation failed"
    };

    EXPECT_FALSE(result.frameId().has_value());
    ASSERT_TRUE(result.physicalAddress().has_value());
    EXPECT_EQ(
        result.physicalAddress().value(),
        PhysicalAddress{0x1000});
}

TEST(MemoryAccessResultTest, PhysicalAddressCanBeAbsent)
{
    const MemoryAccessResult result{
        false,
        false,
        false,
        FrameId{4},
        std::nullopt,
        false,
        "Address translation failed"
    };

    ASSERT_TRUE(result.frameId().has_value());
    EXPECT_EQ(result.frameId().value(), FrameId{4});
    EXPECT_FALSE(result.physicalAddress().has_value());
}

TEST(MemoryAccessResultTest, FrameIdAndPhysicalAddressCanBothBeAbsent)
{
    const MemoryAccessResult result{
        false,
        false,
        false,
        std::nullopt,
        std::nullopt,
        false,
        "Invalid process"
    };

    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());
}

// -----------------------------------------------------------------------------
// Error information
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, ErrorInformationDefaultsToEmpty)
{
    const MemoryAccessResult result{
        true,
        false,
        false,
        FrameId{1},
        PhysicalAddress{0x1000},
        false
    };

    EXPECT_TRUE(result.errorInformation().empty());
}

TEST(MemoryAccessResultTest, ErrorInformationPreservesExactMessage)
{
    const std::string message{
        "ProcessId=100: address outside configured virtual address space"
    };

    const MemoryAccessResult result{
        false,
        false,
        false,
        std::nullopt,
        std::nullopt,
        false,
        message
    };

    EXPECT_EQ(result.errorInformation(), message);
}

// -----------------------------------------------------------------------------
// State combinations
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, PageFaultWithoutReplacementRepresentsFreeFramePath)
{
    const MemoryAccessResult result{
        true,
        true,
        false,
        FrameId{0},
        PhysicalAddress{0x0010},
        false
    };

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
}

TEST(MemoryAccessResultTest, PageFaultWithReplacementButCleanVictimIsValid)
{
    const MemoryAccessResult result{
        true,
        true,
        true,
        FrameId{1},
        PhysicalAddress{0x1010},
        false
    };

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());
    EXPECT_FALSE(result.dirtyEviction());
}

TEST(MemoryAccessResultTest, PageFaultWithReplacementAndDirtyEvictionIsValid)
{
    const MemoryAccessResult result{
        true,
        true,
        true,
        FrameId{1},
        PhysicalAddress{0x1010},
        true
    };

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.pageFault());
    EXPECT_TRUE(result.pageReplacement());
    EXPECT_TRUE(result.dirtyEviction());
}

// -----------------------------------------------------------------------------
// Type-safety
// -----------------------------------------------------------------------------

TEST(MemoryAccessResultTest, FrameIdAndPhysicalAddressUseDistinctTypes)
{
    static_assert(
        !std::is_same_v<FrameId, PhysicalAddress>);

    static_assert(
        !std::is_convertible_v<FrameId, PhysicalAddress>);

    static_assert(
        !std::is_convertible_v<PhysicalAddress, FrameId>);

    SUCCEED();
}

} // namespace