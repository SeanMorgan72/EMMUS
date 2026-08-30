#include <concepts>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <gtest/gtest.h>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace
{

using emmus::memory::identifiers::FrameId;
using emmus::memory::identifiers::PageId;
using emmus::memory::identifiers::ProcessId;

// ============================================================================
// Compile-Time Type Verification
// ============================================================================

static_assert(std::is_same_v<
    ProcessId::ValueType,
    std::uint64_t>);

static_assert(std::is_same_v<
    PageId::ValueType,
    std::uint64_t>);

static_assert(std::is_same_v<
    FrameId::ValueType,
    std::uint64_t>);


// ============================================================================
// Compile-Time Strong-Type Verification
// ============================================================================

static_assert(!std::is_same_v<ProcessId, PageId>);
static_assert(!std::is_same_v<ProcessId, FrameId>);
static_assert(!std::is_same_v<PageId, FrameId>);


// ============================================================================
// Construction Verification
// ============================================================================

static_assert(std::constructible_from<
    ProcessId,
    std::uint64_t>);

static_assert(std::constructible_from<
    PageId,
    std::uint64_t>);

static_assert(std::constructible_from<
    FrameId,
    std::uint64_t>);


// ============================================================================
// Conversion Verification
// ============================================================================
//
// The identifier constructor is explicit, so the underlying integer type
// cannot be implicitly converted into an identifier.
//
// Likewise, one identifier type cannot implicitly convert into another.
//
// ============================================================================

static_assert(!std::convertible_to<
    std::uint64_t,
    ProcessId>);

static_assert(!std::convertible_to<
    std::uint64_t,
    PageId>);

static_assert(!std::convertible_to<
    std::uint64_t,
    FrameId>);


static_assert(!std::convertible_to<
    ProcessId,
    PageId>);

static_assert(!std::convertible_to<
    ProcessId,
    FrameId>);

static_assert(!std::convertible_to<
    PageId,
    ProcessId>);

static_assert(!std::convertible_to<
    PageId,
    FrameId>);

static_assert(!std::convertible_to<
    FrameId,
    ProcessId>);

static_assert(!std::convertible_to<
    FrameId,
    PageId>);


// ============================================================================
// ProcessId Tests
// ============================================================================

TEST(
    MemoryObjectIdsTest,
    ProcessIdStoresAndReturnsValue
)
{
    constexpr ProcessId id{42};

    static_assert(id.value() == 42);

    EXPECT_EQ(
        id.value(),
        42
    );
}


TEST(
    MemoryObjectIdsTest,
    ProcessIdsWithSameValueAreEqual
)
{
    constexpr ProcessId first{10};
    constexpr ProcessId same{10};

    EXPECT_EQ(
        first,
        same
    );
}


TEST(
    MemoryObjectIdsTest,
    ProcessIdsWithDifferentValuesAreNotEqual
)
{
    constexpr ProcessId first{10};
    constexpr ProcessId different{20};

    EXPECT_NE(
        first,
        different
    );
}


// ============================================================================
// PageId Tests
// ============================================================================

TEST(
    MemoryObjectIdsTest,
    PageIdStoresAndReturnsValue
)
{
    constexpr PageId id{17};

    static_assert(id.value() == 17);

    EXPECT_EQ(
        id.value(),
        17
    );
}


TEST(
    MemoryObjectIdsTest,
    PageIdsWithSameValueAreEqual
)
{
    constexpr PageId first{10};
    constexpr PageId same{10};

    EXPECT_EQ(
        first,
        same
    );
}


TEST(
    MemoryObjectIdsTest,
    PageIdsWithDifferentValuesAreNotEqual
)
{
    constexpr PageId first{10};
    constexpr PageId different{20};

    EXPECT_NE(
        first,
        different
    );
}


// ============================================================================
// FrameId Tests
// ============================================================================

TEST(
    MemoryObjectIdsTest,
    FrameIdStoresAndReturnsValue
)
{
    constexpr FrameId id{7};

    static_assert(id.value() == 7);

    EXPECT_EQ(
        id.value(),
        7
    );
}


TEST(
    MemoryObjectIdsTest,
    FrameIdsWithSameValueAreEqual
)
{
    constexpr FrameId first{10};
    constexpr FrameId same{10};

    EXPECT_EQ(
        first,
        same
    );
}


TEST(
    MemoryObjectIdsTest,
    FrameIdsWithDifferentValuesAreNotEqual
)
{
    constexpr FrameId first{10};
    constexpr FrameId different{20};

    EXPECT_NE(
        first,
        different
    );
}


// ============================================================================
// Standard Library Integration
// ============================================================================

TEST(
    MemoryObjectIdsTest,
    IdentifiersSupportUnorderedSets
)
{
    const ProcessId processId{100};
    const PageId pageId{200};
    const FrameId frameId{300};

    std::unordered_set<ProcessId> processes;
    std::unordered_set<PageId> pages;
    std::unordered_set<FrameId> frames;

    processes.insert(processId);
    pages.insert(pageId);
    frames.insert(frameId);

    EXPECT_TRUE(
        processes.contains(processId)
    );

    EXPECT_TRUE(
        pages.contains(pageId)
    );

    EXPECT_TRUE(
        frames.contains(frameId)
    );
}


TEST(
    MemoryObjectIdsTest,
    IdentifiersCanBeUsedAsUnorderedMapKeys
)
{
    std::unordered_map<
        ProcessId,
        std::uint64_t
    > processValues;

    std::unordered_map<
        PageId,
        std::uint64_t
    > pageValues;

    std::unordered_map<
        FrameId,
        std::uint64_t
    > frameValues;


    processValues.emplace(
        ProcessId{1},
        100
    );

    pageValues.emplace(
        PageId{2},
        200
    );

    frameValues.emplace(
        FrameId{3},
        300
    );


    EXPECT_EQ(
        processValues.at(ProcessId{1}),
        100
    );

    EXPECT_EQ(
        pageValues.at(PageId{2}),
        200
    );

    EXPECT_EQ(
        frameValues.at(FrameId{3}),
        300
    );
}


// ============================================================================
// Representation Tests
// ============================================================================

TEST(
    MemoryObjectIdsTest,
    IdentifiersHaveUnderlyingValueSize
)
{
    EXPECT_EQ(
        sizeof(ProcessId),
        sizeof(ProcessId::ValueType)
    );

    EXPECT_EQ(
        sizeof(PageId),
        sizeof(PageId::ValueType)
    );

    EXPECT_EQ(
        sizeof(FrameId),
        sizeof(FrameId::ValueType)
    );
}

} // namespace