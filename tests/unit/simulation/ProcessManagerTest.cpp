#include <gtest/gtest.h>

#include "emmus/simulation/ProcessManager.hpp"

namespace emmus::simulation
{

TEST(ProcessManagerTest, CreatesProcessSuccessfully)
{
    ProcessManager manager;

    Process& process = manager.createProcess(
        16,
        4096
    );

    EXPECT_EQ(
        process.processId().value(),
        1ULL
    );

    EXPECT_EQ(
        process.state(),
        ProcessState::Created
    );

    EXPECT_EQ(
        manager.processCount(),
        1U
    );
}

TEST(ProcessManagerTest, CreatesMultipleProcesses)
{
    ProcessManager manager;

    Process& first = manager.createProcess(16, 4096);
    Process& second = manager.createProcess(32, 4096);
    Process& third = manager.createProcess(64, 4096);

    EXPECT_EQ(manager.processCount(), 3U);

    EXPECT_NE(
        first.processId(),
        second.processId()
    );

    EXPECT_NE(
        first.processId(),
        third.processId()
    );

    EXPECT_NE(
        second.processId(),
        third.processId()
    );
}

TEST(ProcessManagerTest, GeneratesSequentialUniqueProcessIds)
{
    ProcessManager manager;

    const Process& first = manager.createProcess(1, 4096);
    const Process& second = manager.createProcess(1, 4096);
    const Process& third = manager.createProcess(1, 4096);

    EXPECT_EQ(first.processId().value(), 1ULL);
    EXPECT_EQ(second.processId().value(), 2ULL);
    EXPECT_EQ(third.processId().value(), 3ULL);
}

TEST(ProcessManagerTest, DoesNotCreateProcessForZeroPageCount)
{
    ProcessManager manager;

    EXPECT_THROW(
        manager.createProcess(0, 4096),
        std::invalid_argument
    );

    EXPECT_EQ(
        manager.processCount(),
        0U
    );
}

TEST(ProcessManagerTest, DoesNotCreateProcessForZeroPageSize)
{
    ProcessManager manager;

    EXPECT_THROW(
        manager.createProcess(16, 0),
        std::invalid_argument
    );

    EXPECT_EQ(
        manager.processCount(),
        0U
    );
}

TEST(ProcessManagerTest, FailedCreationDoesNotConsumeProcessId)
{
    ProcessManager manager;

    EXPECT_THROW(
        manager.createProcess(0, 4096),
        std::invalid_argument
    );

    Process& process = manager.createProcess(
        16,
        4096
    );

    EXPECT_EQ(
        process.processId().value(),
        1ULL
    );
}

TEST(ProcessManagerTest, FindsExistingProcess)
{
    ProcessManager manager;

    Process& created = manager.createProcess(
        16,
        4096
    );

    Process* found = manager.findProcess(
        created.processId()
    );

    ASSERT_NE(found, nullptr);
    EXPECT_EQ(
        found->processId(),
        created.processId()
    );
}

TEST(ProcessManagerTest, ReturnsNullForUnknownProcess)
{
    ProcessManager manager;

    EXPECT_EQ(
        manager.findProcess(
            memory::identifiers::ProcessId{999}
        ),
        nullptr
    );
}

TEST(ProcessManagerTest, ContainsCreatedProcess)
{
    ProcessManager manager;

    Process& process = manager.createProcess(
        16,
        4096
    );

    EXPECT_TRUE(
        manager.contains(process.processId())
    );
}

TEST(ProcessManagerTest, ClearRemovesAllProcesses)
{
    ProcessManager manager;

    manager.createProcess(16, 4096);
    manager.createProcess(32, 4096);

    EXPECT_EQ(manager.processCount(), 2U);

    manager.clear();

    EXPECT_EQ(manager.processCount(), 0U);
}

TEST(ProcessManagerTest, ClearResetsProcessIdGeneration)
{
    ProcessManager manager;

    Process& first = manager.createProcess(16, 4096);

    EXPECT_EQ(first.processId().value(), 1ULL);

    manager.clear();

    Process& second = manager.createProcess(16, 4096);

    EXPECT_EQ(second.processId().value(), 1ULL);
}

} // namespace emmus::simulation