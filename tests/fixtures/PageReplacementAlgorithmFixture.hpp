#pragma once

#include "TestFixture.hpp"

namespace emmus::test {

/**
 * @brief Common fixture for page-replacement algorithm tests.
 *
 * The fixture deliberately does not depend on a specific replacement
 * algorithm. Individual tests should construct the policy they are testing.
 *
 * This keeps FIFO, LRU, Clock, and Optimal tests independent.
 */
class PageReplacementAlgorithmFixture : public TestFixture
{
protected:

    using FrameId = unsigned long long;

    static constexpr FrameId kFrame0 = 0;
    static constexpr FrameId kFrame1 = 1;
    static constexpr FrameId kFrame2 = 2;
    static constexpr FrameId kFrame3 = 3;
};

} // namespace emmus::test