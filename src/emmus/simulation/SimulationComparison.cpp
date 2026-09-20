#include "emmus/simulation/SimulationComparison.hpp"

#include <utility>

namespace emmus::simulation
{

SimulationComparison::SimulationComparison(
    emmus::infrastructure::configuration::BenchmarkConfiguration benchmark)
    : benchmark_(std::move(benchmark))
{
}

const emmus::infrastructure::configuration::BenchmarkConfiguration&
SimulationComparison::benchmark() const noexcept
{
    return benchmark_;
}

emmus::statistics::PageFaultComparison SimulationComparison::run() const
{
    emmus::statistics::PageFaultComparison comparison;

    for (const auto policy : supportedPolicies())
    {
        const auto configuration =
            benchmark_.withReplacementPolicy(policy)
                .toSimulationConfiguration();

        const auto result = Simulation(configuration).run();

        comparison.recordResult(
            policy,
            result.pageFaultStatistics());
    }

    return comparison;
}

std::vector<SimulationComparison::PolicyType>
SimulationComparison::supportedPolicies() noexcept
{
    return {
        PolicyType::FIFO,
        PolicyType::LRU,
        PolicyType::CLOCK,
        PolicyType::OPTIMAL
    };
}

} // namespace emmus::simulation
