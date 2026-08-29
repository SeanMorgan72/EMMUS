#include <gtest/gtest.h>

#include "fixtures/TestFixture.hpp"

namespace {

class FrameworkSmokeTest : public emmus::test::TestFixture
{
};

TEST_F(
    FrameworkSmokeTest,
    GoogleTestAndCMakeCTestAreOperational
)
{
    SUCCEED();
}


TEST_F(
    FrameworkSmokeTest,
    Cpp23IsEnabled
)
{
    #if __cplusplus >= 202100L

    SUCCEED();

#else

    FAIL()
        << "EMMUS requires C++23 or later. "
        << "__cplusplus=" << __cplusplus;

#endif
}

} // namespace