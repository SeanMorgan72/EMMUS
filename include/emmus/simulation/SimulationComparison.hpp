#pragma once

#include <vector>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/BenchmarkConfiguration.hpp"
#include "emmus/simulation/Simulation.hpp"
#include "emmus/statistics/PageFaultComparison.hpp"

namespace emmus::simulation
{

/**
 * @brief Executes the same benchmark across each supported replacement policy.
 *
 * SimulationComparison preserves one benchmark configuration and reruns the
 * simulation for FIFO, LRU, Clock, and Optimal replacement policies. The result
 * can then be compared directly by fault count and fault rate.
 */
class SimulationComparison final
{
public:
    using PolicyType =
        emmus::algorithms::replacement::PageReplacementPolicyType;

    explicit SimulationComparison(
        emmus::infrastructure::configuration::BenchmarkConfiguration benchmark);

    [[nodiscard]] const emmus::infrastructure::configuration::BenchmarkConfiguration&
    benchmark() const noexcept;

    [[nodiscard]] emmus::statistics::PageFaultComparison run() const;

private:
    static std::vector<PolicyType> supportedPolicies() noexcept;

    emmus::infrastructure::configuration::BenchmarkConfiguration benchmark_;
};

} // namespace emmus::simulation
