#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/infrastructure/configuration/SimulationConfigurationValidator.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"

using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::infrastructure::configuration::SimulationConfiguration;
using emmus::infrastructure::configuration::SimulationConfigurationValidator;
using emmus::infrastructure::configuration::WorkloadType;
using emmus::memory::access::FrameCount;
using emmus::memory::access::PageSize;

namespace {

SimulationConfiguration makeConfiguration(
    std::size_t processCount = 2,
    std::size_t pageCountPerProcess = 8,
    std::size_t memoryAccessCount = 100,
    std::size_t frameCount = 4,
    PageReplacementPolicyType replacementPolicy =
        PageReplacementPolicyType::FIFO,
    WorkloadType workloadType = WorkloadType::Random,
    std::uint64_t randomSeed = 12345)
{
    return SimulationConfiguration(
        PageSize{4096},
        FrameCount{frameCount},
        processCount,
        pageCountPerProcess,
        replacementPolicy,
        workloadType,
        memoryAccessCount,
        randomSeed);
}

} // namespace

TEST(SimulationConfigurationValidatorTest, AcceptsValidConfiguration)
{
    const auto configuration = makeConfiguration();

    const auto diagnostics =
        SimulationConfigurationValidator::validate(configuration);

    EXPECT_TRUE(diagnostics.empty());
    EXPECT_TRUE(
        SimulationConfigurationValidator::isValid(configuration));
}

TEST(SimulationConfigurationValidatorTest, RejectsZeroProcessCount)
{
    const auto configuration =
        makeConfiguration(0);

    const auto diagnostics =
        SimulationConfigurationValidator::validate(configuration);

    EXPECT_FALSE(diagnostics.empty());
    EXPECT_FALSE(
        SimulationConfigurationValidator::isValid(configuration));

    EXPECT_NE(
        std::find(
            diagnostics.begin(),
            diagnostics.end(),
            "Process count must be greater than zero."),
        diagnostics.end());
}

TEST(SimulationConfigurationValidatorTest, RejectsZeroPageCount)
{
    const auto configuration =
        makeConfiguration(2, 0);

    const auto diagnostics =
        SimulationConfigurationValidator::validate(configuration);

    EXPECT_FALSE(diagnostics.empty());
    EXPECT_FALSE(
        SimulationConfigurationValidator::isValid(configuration));

    EXPECT_NE(
        std::find(
            diagnostics.begin(),
            diagnostics.end(),
            "Page count per process must be greater than zero."),
        diagnostics.end());
}

TEST(SimulationConfigurationValidatorTest, RejectsZeroAccessCount)
{
    const auto configuration =
        makeConfiguration(2, 8, 0);

    const auto diagnostics =
        SimulationConfigurationValidator::validate(configuration);

    EXPECT_FALSE(diagnostics.empty());
    EXPECT_FALSE(
        SimulationConfigurationValidator::isValid(configuration));

    EXPECT_NE(
        std::find(
            diagnostics.begin(),
            diagnostics.end(),
            "Memory access count must be greater than zero."),
        diagnostics.end());
}

TEST(SimulationConfigurationValidatorTest, RejectsZeroFrameCount)
{
    EXPECT_THROW(
        {
            const auto configuration =
                makeConfiguration(2, 8, 100, 0);

            static_cast<void>(
                SimulationConfigurationValidator::validate(
                    configuration));
        },
        std::invalid_argument);
}

TEST(SimulationConfigurationValidatorTest, RejectsInvalidReplacementPolicy)
{
    const auto configuration =
        makeConfiguration(
            2,
            8,
            100,
            4,
            static_cast<PageReplacementPolicyType>(255));

    const auto diagnostics =
        SimulationConfigurationValidator::validate(configuration);

    EXPECT_FALSE(diagnostics.empty());
    EXPECT_FALSE(
        SimulationConfigurationValidator::isValid(configuration));

    EXPECT_NE(
        std::find(
            diagnostics.begin(),
            diagnostics.end(),
            "Unsupported page replacement policy."),
        diagnostics.end());
}

TEST(SimulationConfigurationValidatorTest, AcceptsAllSupportedReplacementPolicies)
{
    constexpr PageReplacementPolicyType policies[] = {
        PageReplacementPolicyType::FIFO,
        PageReplacementPolicyType::LRU,
        PageReplacementPolicyType::CLOCK,
        PageReplacementPolicyType::OPTIMAL
    };

    for (const auto policy : policies)
    {
        const auto configuration =
            makeConfiguration(
                2,
                8,
                100,
                4,
                policy);

        EXPECT_TRUE(
            SimulationConfigurationValidator::isValid(configuration));
    }
}