#pragma once

#include "TestFixture.hpp"

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::test
{

/**
 * @brief Base fixture for memory-management tests.
 *
 * Provides common production identifier types and deterministic test
 * identifiers for memory-management tests.
 *
 * The fixture uses the production EMMUS identifier types directly so that
 * test code cannot accidentally bypass the type-safety guarantees provided
 * by MemoryObjectIds.hpp.
 */
class MemoryManagerFixture : public TestFixture
{
protected:

    using ProcessId =
        emmus::memory::identifiers::ProcessId;

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;


    // -----------------------------------------------------------------------
    // Process IDs
    // -----------------------------------------------------------------------

    static constexpr ProcessId kProcess1{
        100
    };

    static constexpr ProcessId kProcess2{
        200
    };

    static constexpr ProcessId kProcess3{
        300
    };


    // -----------------------------------------------------------------------
    // Page IDs
    // -----------------------------------------------------------------------

    static constexpr PageId kPage0{
        0
    };

    static constexpr PageId kPage1{
        1
    };

    static constexpr PageId kPage2{
        2
    };

    static constexpr PageId kPage3{
        3
    };


    // -----------------------------------------------------------------------
    // Frame IDs
    // -----------------------------------------------------------------------

    static constexpr FrameId kFrame0{
        0
    };

    static constexpr FrameId kFrame1{
        1
    };

    static constexpr FrameId kFrame2{
        2
    };

    static constexpr FrameId kFrame3{
        3
    };
};

} // namespace emmus::test