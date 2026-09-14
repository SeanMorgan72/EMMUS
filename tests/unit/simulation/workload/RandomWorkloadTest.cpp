#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/simulation/workload/RandomWorkload.hpp"

namespace {

using emmus::memory::access::AccessSequenceNumber;
using emmus::memory::access::MemoryAccess;
using emmus::memory::access::MemoryAccessOperation;
using emmus::memory::access::PageSize;
using emmus::memory::identifiers::ProcessId;
using emmus::simulation::workload::RandomWorkload;

std::vector<MemoryAccess> collectAccesses(RandomWorkload& workload) {
    std::vector<MemoryAccess> accesses;

    while (workload.hasNext()) {
        accesses.push_back(workload.nextAccess());
    }

    return accesses;
}

TEST(RandomWorkloadTest, GeneratesRequestedNumberOfAccesses) {
    constexpr std::size_t accessCount = 100;

    RandomWorkload workload(
        ProcessId{1},
        16,
        PageSize{4096},
        accessCount,
        42);

    EXPECT_EQ(workload.size(), accessCount);

    std::size_t observedCount = 0;

    while (workload.hasNext()) {
        static_cast<void>(workload.nextAccess());
        ++observedCount;
    }

    EXPECT_EQ(observedCount, accessCount);
    EXPECT_FALSE(workload.hasNext());
}

TEST(RandomWorkloadTest, GeneratesAddressesWithinConfiguredPageRange) {
    constexpr std::size_t pageCount = 8;
    constexpr std::uint64_t pageSize = 4096;
    constexpr std::size_t accessCount = 1000;

    RandomWorkload workload(
        ProcessId{1},
        pageCount,
        PageSize{pageSize},
        accessCount,
        42);

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();
        const auto address = access.virtualAddress().value();

        EXPECT_LT(
            address / pageSize,
            static_cast<std::uint64_t>(pageCount));
    }
}

TEST(RandomWorkloadTest, GeneratesPageAlignedAddresses) {
    constexpr std::uint64_t pageSize = 4096;

    RandomWorkload workload(
        ProcessId{1},
        32,
        PageSize{pageSize},
        500,
        42);

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_EQ(
            access.virtualAddress().value() % pageSize,
            0);
    }
}

TEST(RandomWorkloadTest, PropagatesConfiguredProcessId) {
    constexpr ProcessId processId{17};

    RandomWorkload workload(
        processId,
        8,
        PageSize{4096},
        100,
        42);

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_EQ(access.processId(), processId);
    }
}

TEST(RandomWorkloadTest, GeneratesReadOperations) {
    RandomWorkload workload(
        ProcessId{1},
        8,
        PageSize{4096},
        100,
        42);

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_EQ(
            access.operation(),
            MemoryAccessOperation::Read);
    }
}

TEST(RandomWorkloadTest, GeneratesSequentialAccessSequenceNumbers) {
    constexpr std::size_t accessCount = 100;

    RandomWorkload workload(
        ProcessId{1},
        16,
        PageSize{4096},
        accessCount,
        42);

    std::size_t expectedSequenceNumber = 0;

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_EQ(
            access.sequenceNumber(),
            AccessSequenceNumber{
                static_cast<std::uint64_t>(expectedSequenceNumber)});

        ++expectedSequenceNumber;
    }

    EXPECT_EQ(expectedSequenceNumber, accessCount);
}

TEST(RandomWorkloadTest, SameSeedProducesIdenticalSequence) {
    constexpr std::size_t accessCount = 200;
    constexpr std::uint64_t seed = 123456789;

    RandomWorkload first(
        ProcessId{1},
        32,
        PageSize{4096},
        accessCount,
        seed);

    RandomWorkload second(
        ProcessId{1},
        32,
        PageSize{4096},
        accessCount,
        seed);

    const auto firstAccesses = collectAccesses(first);
    const auto secondAccesses = collectAccesses(second);

    ASSERT_EQ(firstAccesses.size(), secondAccesses.size());
    EXPECT_EQ(firstAccesses, secondAccesses);
}

TEST(RandomWorkloadTest, DifferentSeedsProduceDifferentSequences) {
    constexpr std::size_t accessCount = 200;

    RandomWorkload first(
        ProcessId{1},
        32,
        PageSize{4096},
        accessCount,
        12345);

    RandomWorkload second(
        ProcessId{1},
        32,
        PageSize{4096},
        accessCount,
        67890);

    const auto firstAccesses = collectAccesses(first);
    const auto secondAccesses = collectAccesses(second);

    ASSERT_EQ(firstAccesses.size(), secondAccesses.size());

    EXPECT_NE(firstAccesses, secondAccesses);
}

TEST(RandomWorkloadTest, ResetReproducesOriginalSequence) {
    constexpr std::size_t accessCount = 200;

    RandomWorkload workload(
        ProcessId{1},
        32,
        PageSize{4096},
        accessCount,
        42);

    const auto firstSequence = collectAccesses(workload);

    EXPECT_FALSE(workload.hasNext());

    workload.reset();

    EXPECT_TRUE(workload.hasNext());

    const auto secondSequence = collectAccesses(workload);

    EXPECT_EQ(firstSequence, secondSequence);
}

TEST(RandomWorkloadTest, SizeRemainsConstantAfterConsumptionAndReset) {
    constexpr std::size_t accessCount = 50;

    RandomWorkload workload(
        ProcessId{1},
        8,
        PageSize{4096},
        accessCount,
        42);

    EXPECT_EQ(workload.size(), accessCount);

    static_cast<void>(collectAccesses(workload));

    EXPECT_EQ(workload.size(), accessCount);

    workload.reset();

    EXPECT_EQ(workload.size(), accessCount);
}

TEST(RandomWorkloadTest, NextAccessThrowsWhenWorkloadIsExhausted) {
    RandomWorkload workload(
        ProcessId{1},
        4,
        PageSize{4096},
        1,
        42);

    ASSERT_TRUE(workload.hasNext());

    static_cast<void>(workload.nextAccess());

    EXPECT_FALSE(workload.hasNext());

    EXPECT_THROW(
        static_cast<void>(workload.nextAccess()),
        std::out_of_range);
}

TEST(RandomWorkloadTest, RejectsZeroPageCount) {
    EXPECT_THROW(
        RandomWorkload(
            ProcessId{1},
            0,
            PageSize{4096},
            10,
            42),
        std::invalid_argument);
}

TEST(RandomWorkloadTest, RejectsZeroAccessCount) {
    EXPECT_THROW(
        RandomWorkload(
            ProcessId{1},
            8,
            PageSize{4096},
            0,
            42),
        std::invalid_argument);
}

TEST(RandomWorkloadTest, RejectsAddressSpaceOverflow) {
    constexpr auto maximum =
        std::numeric_limits<std::uint64_t>::max();

    const auto pageSizeValue = maximum / 2 + 1;

    ASSERT_GT(pageSizeValue, 0);

    EXPECT_THROW(
        RandomWorkload(
            ProcessId{1},
            3,
            PageSize{pageSizeValue},
            1,
            42),
        std::invalid_argument);
}

TEST(RandomWorkloadTest, AllowsLargestRepresentablePageBase) {
    constexpr auto maximum =
        std::numeric_limits<std::uint64_t>::max();

    const auto pageSizeValue = maximum / 2;

    RandomWorkload workload(
        ProcessId{1},
        3,
        PageSize{pageSizeValue},
        100,
        42);

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_LE(
            access.virtualAddress().value(),
            maximum);
    }
}

} // namespace