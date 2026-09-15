#include "emmus/simulation/workload/LocalityWorkload.hpp"

#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace emmus::simulation::workload {
namespace {

using memory::access::AccessSequenceNumber;
using memory::access::MemoryAccess;
using memory::access::PageSize;
using memory::access::VirtualAddress;
using memory::identifiers::ProcessId;

LocalityWorkload makeWorkload(
    ProcessId processId = ProcessId{7},
    std::size_t pageCount = 32U,
    std::size_t accessCount = 100U,
    double temporalStrength = 0.75,
    double spatialStrength = 0.75,
    std::size_t workingSetSize = 8U,
    std::uint64_t seed = 12345U) {

    return LocalityWorkload(
        processId,
        pageCount,
        PageSize{4096U},
        accessCount,
        temporalStrength,
        spatialStrength,
        workingSetSize,
        seed);
}

std::vector<MemoryAccess> collect(
    LocalityWorkload& workload) {

    std::vector<MemoryAccess> accesses;
    accesses.reserve(workload.size());

    while (workload.hasNext()) {
        accesses.push_back(workload.nextAccess());
    }

    return accesses;
}

} // namespace

TEST(LocalityWorkloadTest, GeneratesRequestedNumberOfAccesses) {
    auto workload = makeWorkload();

    EXPECT_EQ(workload.size(), 100U);

    std::size_t count = 0U;

    while (workload.hasNext()) {
        static_cast<void>(workload.nextAccess());
        ++count;
    }

    EXPECT_EQ(count, 100U);
}

TEST(LocalityWorkloadTest, GeneratesAddressesWithinConfiguredPageRange) {
    constexpr std::size_t pageCount = 32U;
    constexpr std::size_t workingSetSize = 8U;
    constexpr std::uint64_t pageSize = 4096U;

    auto workload = makeWorkload(
        ProcessId{1},
        pageCount,
        200U,
        0.5,
        0.5,
        workingSetSize,
        99U);

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();
        const auto pageIndex =
            access.virtualAddress().value() / pageSize;

        EXPECT_LT(pageIndex, pageCount);
        EXPECT_GE(
            pageIndex,
            static_cast<std::uint64_t>(0U));
        EXPECT_LT(
            pageIndex,
            static_cast<std::uint64_t>(pageCount));
    }
}

TEST(LocalityWorkloadTest, AddressesArePageAligned) {
    auto workload = makeWorkload();

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_EQ(
            access.virtualAddress().value() % 4096U,
            0U);
    }
}

TEST(LocalityWorkloadTest, UsesConfiguredProcessId) {
    constexpr ProcessId expectedProcessId{42};

    auto workload = makeWorkload(
        expectedProcessId);

    while (workload.hasNext()) {
        EXPECT_EQ(
            workload.nextAccess().processId(),
            expectedProcessId);
    }
}

TEST(LocalityWorkloadTest, GeneratesReadOperations) {
    auto workload = makeWorkload();

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_TRUE(access.isRead());
        EXPECT_FALSE(access.isWrite());
    }
}

TEST(LocalityWorkloadTest, GeneratesSequentialSequenceNumbers) {
    auto workload = makeWorkload(
        ProcessId{1},
        32U,
        50U);

    std::uint64_t expectedSequence = 0U;

    while (workload.hasNext()) {
        const auto access = workload.nextAccess();

        EXPECT_EQ(
            access.sequenceNumber().value(),
            expectedSequence);

        ++expectedSequence;
    }
}

TEST(LocalityWorkloadTest, SameSeedProducesIdenticalSequence) {
    auto first = makeWorkload(
        ProcessId{5},
        64U,
        200U,
        0.7,
        0.8,
        12U,
        123456U);

    auto second = makeWorkload(
        ProcessId{5},
        64U,
        200U,
        0.7,
        0.8,
        12U,
        123456U);

    EXPECT_EQ(
        collect(first),
        collect(second));
}

TEST(LocalityWorkloadTest, DifferentSeedsCanProduceDifferentSequence) {
    auto first = makeWorkload(
        ProcessId{5},
        64U,
        200U,
        0.7,
        0.8,
        12U,
        123456U);

    auto second = makeWorkload(
        ProcessId{5},
        64U,
        200U,
        0.7,
        0.8,
        12U,
        654321U);

    EXPECT_NE(
        collect(first),
        collect(second));
}

TEST(LocalityWorkloadTest, ResetReproducesGeneratedSequence) {
    auto workload = makeWorkload();

    const auto firstSequence =
        collect(workload);

    workload.reset();

    const auto secondSequence =
        collect(workload);

    EXPECT_EQ(
        firstSequence,
        secondSequence);
}

TEST(LocalityWorkloadTest, TemporalOnlyWorkloadReusesRecentPages) {
    auto workload = makeWorkload(
        ProcessId{1},
        64U,
        200U,
        1.0,
        0.0,
        8U,
        123U);

    const auto accesses =
        collect(workload);

    ASSERT_FALSE(accesses.empty());

    bool observedReuse = false;

    for (std::size_t index = 1U;
         index < accesses.size();
         ++index) {

        if (accesses[index].virtualAddress() ==
            accesses[index - 1U].virtualAddress()) {

            observedReuse = true;
            break;
        }
    }

    EXPECT_TRUE(observedReuse);
}

TEST(LocalityWorkloadTest, SpatialOnlyWorkloadProducesNearbyPages) {
    auto workload = makeWorkload(
        ProcessId{1},
        64U,
        200U,
        0.0,
        1.0,
        8U,
        456U);

    const auto accesses =
        collect(workload);

    ASSERT_GT(accesses.size(), 1U);

    for (std::size_t index = 1U;
         index < accesses.size();
         ++index) {

        const auto current =
            accesses[index].virtualAddress().value() /
            4096U;

        const auto previous =
            accesses[index - 1U].virtualAddress().value() /
            4096U;

        const auto distance =
            current > previous
                ? current - previous
                : previous - current;

        EXPECT_LE(distance, 2U);
    }
}

TEST(LocalityWorkloadTest, ZeroLocalityStrengthsProduceValidWorkload) {
    auto workload = makeWorkload(
        ProcessId{1},
        64U,
        100U,
        0.0,
        0.0,
        8U,
        99U);

    EXPECT_EQ(workload.size(), 100U);

    EXPECT_NO_THROW({
        while (workload.hasNext()) {
            static_cast<void>(workload.nextAccess());
        }
    });
}

TEST(LocalityWorkloadTest, TemporalAndSpatialLocalityCanBeCombined) {
    auto workload = makeWorkload(
        ProcessId{1},
        64U,
        500U,
        0.5,
        0.5,
        16U,
        9876U);

    EXPECT_EQ(workload.size(), 500U);

    const auto accesses =
        collect(workload);

    EXPECT_EQ(accesses.size(), 500U);
}

TEST(LocalityWorkloadTest, WorkingSetConstrainsGeneratedPages) {
    constexpr std::size_t pageCount = 64U;
    constexpr std::size_t workingSetSize = 8U;

    auto workload = makeWorkload(
        ProcessId{1},
        pageCount,
        500U,
        0.8,
        0.8,
        workingSetSize,
        111U);

    const auto accesses =
        collect(workload);

    ASSERT_FALSE(accesses.empty());

    std::uint64_t minimumPage =
        accesses.front().virtualAddress().value() /
        4096U;

    std::uint64_t maximumPage = minimumPage;

    for (const auto& access : accesses) {
        const auto page =
            access.virtualAddress().value() / 4096U;

        minimumPage =
            std::min(minimumPage, page);

        maximumPage =
            std::max(maximumPage, page);
    }

    EXPECT_LE(
        maximumPage - minimumPage + 1U,
        static_cast<std::uint64_t>(workingSetSize));
}

TEST(LocalityWorkloadTest, RejectsZeroPageCount) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            0U,
            PageSize{4096U},
            10U,
            0.5,
            0.5,
            1U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsZeroAccessCount) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            10U,
            PageSize{4096U},
            0U,
            0.5,
            0.5,
            5U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsZeroWorkingSetSize) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            10U,
            PageSize{4096U},
            10U,
            0.5,
            0.5,
            0U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsWorkingSetLargerThanPageRange) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            10U,
            PageSize{4096U},
            10U,
            0.5,
            0.5,
            11U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsNegativeTemporalStrength) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            10U,
            PageSize{4096U},
            10U,
            -0.1,
            0.5,
            5U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsTemporalStrengthGreaterThanOne) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            10U,
            PageSize{4096U},
            10U,
            1.1,
            0.5,
            5U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsNegativeSpatialStrength) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            10U,
            PageSize{4096U},
            10U,
            0.5,
            -0.1,
            5U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsSpatialStrengthGreaterThanOne) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            10U,
            PageSize{4096U},
            10U,
            0.5,
            1.1,
            5U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, RejectsAddressRangeOverflow) {
    EXPECT_THROW(
        LocalityWorkload(
            ProcessId{1},
            std::numeric_limits<std::uint64_t>::max(),
            PageSize{2U},
            10U,
            0.5,
            0.5,
            10U,
            1U),
        std::invalid_argument);
}

TEST(LocalityWorkloadTest, ThrowsWhenExhausted) {
    auto workload = makeWorkload(
        ProcessId{1},
        10U,
        2U,
        0.5,
        0.5,
        2U,
        1U);

    static_cast<void>(workload.nextAccess());
    static_cast<void>(workload.nextAccess());

    EXPECT_FALSE(workload.hasNext());

    EXPECT_THROW(
        static_cast<void>(workload.nextAccess()),
        std::out_of_range);
}

TEST(LocalityWorkloadTest, SizeDoesNotChangeWhenConsumed) {
    auto workload = makeWorkload(
        ProcessId{1},
        10U,
        20U,
        0.5,
        0.5,
        5U,
        1U);

    EXPECT_EQ(workload.size(), 20U);

    static_cast<void>(workload.nextAccess());

    EXPECT_EQ(workload.size(), 20U);
}

} // namespace emmus::simulation::workload