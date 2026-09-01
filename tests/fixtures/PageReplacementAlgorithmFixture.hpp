#pragma once

#include "TestFixture.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::test
{

/**
 * @brief Common fixture for page-replacement policy tests.
 *
 * The fixture deliberately does not depend on a specific replacement
 * policy or replacement algorithm. Individual tests should construct
 * the policy they are testing.
 *
 * This keeps FIFO, LRU, Clock, Optimal, and future replacement-policy
 * tests independent.
 */
class PageReplacementAlgorithmFixture : public TestFixture
{
protected:

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;


    static constexpr PageId kPage0{0};
    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};
    static constexpr PageId kPage3{3};


    static constexpr FrameId kFrame0{0};
    static constexpr FrameId kFrame1{1};
    static constexpr FrameId kFrame2{2};
    static constexpr FrameId kFrame3{3};
};

} // namespace emmus::test