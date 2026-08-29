#pragma once

#include "TestFixture.hpp"

namespace emmus::test {

/**
 * @brief Base fixture for deterministic simulation scenarios.
 *
 * This fixture will eventually provide reusable configuration and workload
 * construction for system-level simulation tests.
 */
class SimulationFixture : public TestFixture
{
protected:

    static constexpr unsigned kDefaultFrameCount = 4;

    static constexpr unsigned kDefaultPageSize = 4096;
};

} // namespace emmus::test