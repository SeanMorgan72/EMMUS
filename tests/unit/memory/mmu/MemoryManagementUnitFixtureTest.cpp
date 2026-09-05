#include "fixtures/MemoryManagementUnitFixture.hpp"

namespace emmus::tests {

TEST_F(MemoryManagementUnitFixture, CreatesEmptyPageTable)
{
    EXPECT_TRUE(pageTable().empty());
    EXPECT_EQ(pageTable().size(), 0U);
}

TEST_F(MemoryManagementUnitFixture, CreatesPhysicalMemoryWithExpectedCapacity)
{
    EXPECT_EQ(
        physicalMemoryManager().capacity(),
        kFrameCount);
    EXPECT_EQ(
        physicalMemoryManager().freeFrameCount(),
        kFrameCount);
    EXPECT_EQ(
        physicalMemoryManager().allocatedFrameCount(),
        0U);
}

TEST_F(MemoryManagementUnitFixture, CreatesReplacementPolicy)
{
    EXPECT_TRUE(replacementPolicy().chooseVictim().has_value() == false);
}

TEST_F(MemoryManagementUnitFixture, ComponentsStartIndependent)
{
    EXPECT_TRUE(pageTable().empty());
    EXPECT_EQ(physicalMemoryManager().allocatedFrameCount(), 0U);
    EXPECT_FALSE(replacementPolicy().chooseVictim().has_value());
}

} // namespace emmus::tests
