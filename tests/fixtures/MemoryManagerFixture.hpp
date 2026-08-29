#pragma once

#include "TestFixture.hpp"

namespace emmus::test {

/**
 * @brief Base fixture for memory-management tests.
 *
 * This fixture intentionally does not construct concrete production objects
 * until their public interfaces are established.
 *
 * Keeping test data here provides consistent identifiers across tests while
 * avoiding hidden dependencies between individual test cases.
 */
class MemoryManagerFixture : public TestFixture
{
protected:

    using ProcessId = unsigned long long;
    using PageId    = unsigned long long;
    using FrameId   = unsigned long long;

    // -----------------------------------------------------------------------
    // Process IDs
    // -----------------------------------------------------------------------

    static constexpr ProcessId kProcess1 = 100;
    static constexpr ProcessId kProcess2 = 200;
    static constexpr ProcessId kProcess3 = 300;

    // -----------------------------------------------------------------------
    // Page IDs
    // -----------------------------------------------------------------------

    static constexpr PageId kPage0 = 0;
    static constexpr PageId kPage1 = 1;
    static constexpr PageId kPage2 = 2;
    static constexpr PageId kPage3 = 3;

    // -----------------------------------------------------------------------
    // Frame IDs
    // -----------------------------------------------------------------------

    static constexpr FrameId kFrame0 = 0;
    static constexpr FrameId kFrame1 = 1;
    static constexpr FrameId kFrame2 = 2;
    static constexpr FrameId kFrame3 = 3;
};

} // namespace emmus::test