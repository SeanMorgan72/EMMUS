#pragma once

#include <gtest/gtest.h>

namespace emmus::test {

/**
 * @brief Base fixture for EMMUS tests.
 *
 * Provides a common foundation for reusable test fixtures.
 *
 * Tests should avoid global mutable state. Each test should construct or
 * receive the objects required for the behavior under test.
 */
class TestFixture : public ::testing::Test
{
protected:

    void SetUp() override
    {
        // Common test initialization can be added here later.
    }

    void TearDown() override
    {
        // Common test cleanup can be added here later.
    }
};

} // namespace emmus::test