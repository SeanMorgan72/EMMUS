#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace {

using emmus::memory::access::AccessSequenceNumber;
using emmus::memory::access::FrameCount;
using emmus::memory::access::PageOffset;
using emmus::memory::access::PageSize;
using emmus::memory::access::PhysicalAddress;
using emmus::memory::access::VirtualAddress;
using emmus::memory::access::decomposeVirtualAddress;
using emmus::memory::access::makePhysicalAddress;

using emmus::memory::identifiers::FrameId;
using emmus::memory::identifiers::PageId;

// -----------------------------------------------------------------------------
// VirtualAddress
// -----------------------------------------------------------------------------

TEST(VirtualAddressTest, ConstructsWithValidValue)
{
    constexpr std::uint64_t rawAddress = 0x1234U;

    constexpr VirtualAddress address{rawAddress};

    EXPECT_EQ(address.value(), rawAddress);
}

TEST(VirtualAddressTest, SupportsEqualityComparison)
{
    constexpr VirtualAddress first{0x1000U};
    constexpr VirtualAddress same{0x1000U};
    constexpr VirtualAddress different{0x2000U};

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}

// -----------------------------------------------------------------------------
// PhysicalAddress
// -----------------------------------------------------------------------------

TEST(PhysicalAddressTest, ConstructsWithValidValue)
{
    constexpr std::uint64_t rawAddress = 0x4321U;

    constexpr PhysicalAddress address{rawAddress};

    EXPECT_EQ(address.value(), rawAddress);
}

TEST(PhysicalAddressTest, SupportsEqualityComparison)
{
    constexpr PhysicalAddress first{0x1000U};
    constexpr PhysicalAddress same{0x1000U};
    constexpr PhysicalAddress different{0x2000U};

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}

// -----------------------------------------------------------------------------
// PageOffset
// -----------------------------------------------------------------------------

TEST(PageOffsetTest, ConstructsWithValidValue)
{
    constexpr PageOffset offset{123U};

    EXPECT_EQ(offset.value(), 123U);
}

TEST(PageOffsetTest, SupportsEqualityComparison)
{
    constexpr PageOffset first{42U};
    constexpr PageOffset same{42U};
    constexpr PageOffset different{43U};

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}

// -----------------------------------------------------------------------------
// PageSize
// -----------------------------------------------------------------------------

TEST(PageSizeTest, ConstructsWithPositiveValue)
{
    constexpr PageSize pageSize{4096U};

    EXPECT_EQ(pageSize.value(), 4096U);
}

TEST(PageSizeTest, RejectsZero)
{
    EXPECT_THROW(
        {
            [[maybe_unused]] const PageSize pageSize{0U};
        },
        std::invalid_argument);
}

TEST(PageSizeTest, SupportsEqualityComparison)
{
    constexpr PageSize first{4096U};
    constexpr PageSize same{4096U};
    constexpr PageSize different{8192U};

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}

// -----------------------------------------------------------------------------
// FrameCount
// -----------------------------------------------------------------------------

TEST(FrameCountTest, ConstructsWithPositiveValue)
{
    constexpr FrameCount count{16U};

    EXPECT_EQ(count.value(), 16U);
}

TEST(FrameCountTest, RejectsZero)
{
    EXPECT_THROW(
        {
            [[maybe_unused]] const FrameCount count{0U};
        },
        std::invalid_argument);
}

TEST(FrameCountTest, SupportsEqualityComparison)
{
    constexpr FrameCount first{16U};
    constexpr FrameCount same{16U};
    constexpr FrameCount different{32U};

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}

// -----------------------------------------------------------------------------
// AccessSequenceNumber
// -----------------------------------------------------------------------------

TEST(AccessSequenceNumberTest, ConstructsWithValidValue)
{
    constexpr AccessSequenceNumber sequence{12345U};

    EXPECT_EQ(sequence.value(), 12345U);
}

TEST(AccessSequenceNumberTest, SupportsEqualityComparison)
{
    constexpr AccessSequenceNumber first{10U};
    constexpr AccessSequenceNumber same{10U};
    constexpr AccessSequenceNumber different{11U};

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}

// -----------------------------------------------------------------------------
// Virtual address decomposition
// -----------------------------------------------------------------------------

TEST(AddressCalculationTest, DecomposesVirtualAddressIntoPageAndOffset)
{
    constexpr PageSize pageSize{4096U};
    constexpr VirtualAddress virtualAddress{0x12345U};

    const auto [pageId, offset] =
        decomposeVirtualAddress(virtualAddress, pageSize);

    EXPECT_EQ(pageId.value(), 0x12U);
    EXPECT_EQ(offset.value(), 0x345U);
}

TEST(AddressCalculationTest, CalculatesFirstPageWithZeroOffset)
{
    constexpr PageSize pageSize{4096U};
    constexpr VirtualAddress virtualAddress{0U};

    const auto [pageId, offset] =
        decomposeVirtualAddress(virtualAddress, pageSize);

    EXPECT_EQ(pageId, PageId{0U});
    EXPECT_EQ(offset, PageOffset{0U});
}

TEST(AddressCalculationTest, CalculatesPageBoundaryCorrectly)
{
    constexpr PageSize pageSize{4096U};
    constexpr VirtualAddress virtualAddress{4096U};

    const auto [pageId, offset] =
        decomposeVirtualAddress(virtualAddress, pageSize);

    EXPECT_EQ(pageId, PageId{1U});
    EXPECT_EQ(offset, PageOffset{0U});
}

TEST(AddressCalculationTest, CalculatesAddressImmediatelyBeforePageBoundary)
{
    constexpr PageSize pageSize{4096U};
    constexpr VirtualAddress virtualAddress{4095U};

    const auto [pageId, offset] =
        decomposeVirtualAddress(virtualAddress, pageSize);

    EXPECT_EQ(pageId, PageId{0U});
    EXPECT_EQ(offset, PageOffset{4095U});
}

TEST(AddressCalculationTest, HandlesLargeVirtualAddress)
{
    constexpr PageSize pageSize{4096U};
    constexpr VirtualAddress virtualAddress{
        std::numeric_limits<std::uint64_t>::max()};

    const auto [pageId, offset] =
        decomposeVirtualAddress(virtualAddress, pageSize);

    EXPECT_EQ(
        pageId.value(),
        std::numeric_limits<std::uint64_t>::max() / pageSize.value());

    EXPECT_EQ(
        offset.value(),
        std::numeric_limits<std::uint64_t>::max() % pageSize.value());
}

// -----------------------------------------------------------------------------
// Physical address construction
// -----------------------------------------------------------------------------

TEST(AddressCalculationTest, ConstructsPhysicalAddressFromFrameAndOffset)
{
    constexpr PageSize pageSize{4096U};
    constexpr FrameId frameId{3U};
    constexpr PageOffset offset{0x123U};

    constexpr PhysicalAddress physicalAddress =
        makePhysicalAddress(frameId, offset, pageSize);

    EXPECT_EQ(
        physicalAddress.value(),
        (3U * 4096U) + 0x123U);
}

TEST(AddressCalculationTest, PreservesPageOffsetDuringTranslation)
{
    constexpr PageSize pageSize{4096U};
    constexpr VirtualAddress virtualAddress{0x2345U};

    const auto [pageId, offset] =
        decomposeVirtualAddress(virtualAddress, pageSize);

    constexpr FrameId frameId{7U};

    const PhysicalAddress physicalAddress =
        makePhysicalAddress(frameId, offset, pageSize);

    EXPECT_EQ(pageId, PageId{2U});
    EXPECT_EQ(offset, PageOffset{0x345U});

    EXPECT_EQ(
        physicalAddress.value(),
        (7U * pageSize.value()) + offset.value());
}

TEST(AddressCalculationTest, SupportsZeroOffset)
{
    constexpr PageSize pageSize{4096U};
    constexpr FrameId frameId{5U};
    constexpr PageOffset offset{0U};

    constexpr PhysicalAddress physicalAddress =
        makePhysicalAddress(frameId, offset, pageSize);

    EXPECT_EQ(physicalAddress.value(), 5U * 4096U);
}

// -----------------------------------------------------------------------------
// Type-safety expectations
// -----------------------------------------------------------------------------

TEST(TypeSafetyTest, DomainTypesAreDistinct)
{
    static_assert(!std::is_same_v<VirtualAddress, PhysicalAddress>);
    static_assert(!std::is_same_v<VirtualAddress, PageOffset>);
    static_assert(!std::is_same_v<PhysicalAddress, PageOffset>);
    static_assert(!std::is_same_v<PageSize, FrameCount>);
    static_assert(!std::is_same_v<PageId, FrameId>);

    SUCCEED();
}

TEST(TypeSafetyTest, DomainTypesAreNotImplicitlyConvertibleFromEachOther)
{
    static_assert(
        !std::is_convertible_v<VirtualAddress, PhysicalAddress>);

    static_assert(
        !std::is_convertible_v<PhysicalAddress, VirtualAddress>);

    static_assert(
        !std::is_convertible_v<PageOffset, PageSize>);

    static_assert(
        !std::is_convertible_v<PageSize, FrameCount>);

    static_assert(
        !std::is_convertible_v<PageId, FrameId>);

    static_assert(
        !std::is_convertible_v<FrameId, PageId>);

    SUCCEED();
}

// -----------------------------------------------------------------------------
// Compile-time behavior
// -----------------------------------------------------------------------------

TEST(CoreDomainTypesTest, FundamentalOperationsAreConstexpr)
{
    constexpr VirtualAddress virtualAddress{0x1234U};
    constexpr PhysicalAddress physicalAddress{0x5678U};
    constexpr PageOffset offset{0x100U};
    constexpr PageSize pageSize{4096U};
    constexpr FrameCount frameCount{8U};
    constexpr AccessSequenceNumber sequence{42U};

    static_assert(virtualAddress.value() == 0x1234U);
    static_assert(physicalAddress.value() == 0x5678U);
    static_assert(offset.value() == 0x100U);
    static_assert(pageSize.value() == 4096U);
    static_assert(frameCount.value() == 8U);
    static_assert(sequence.value() == 42U);

    SUCCEED();
}

}  // namespace