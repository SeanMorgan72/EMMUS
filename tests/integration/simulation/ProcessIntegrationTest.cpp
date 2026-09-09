#include <gtest/gtest.h>

#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/simulation/ProcessManager.hpp"

namespace emmus::simulation
{

TEST(ProcessIntegrationTest, MultipleProcessesHaveDistinctIdentities)
{
    ProcessManager manager;

    Process& first = manager.createProcess(
        16,
        4096
    );

    Process& second = manager.createProcess(
        16,
        4096
    );

    ASSERT_NE(
        first.processId(),
        second.processId()
    );

    EXPECT_TRUE(
        manager.contains(first.processId())
    );

    EXPECT_TRUE(
        manager.contains(second.processId())
    );
}

TEST(ProcessIntegrationTest, ProcessesMaintainIndependentAddressSpaceMetadata)
{
    ProcessManager manager;

    Process& first = manager.createProcess(
        16,
        4096
    );

    Process& second = manager.createProcess(
        64,
        4096
    );

    EXPECT_EQ(
        first.virtualAddressSpace().pageCount(),
        16ULL
    );

    EXPECT_EQ(
        second.virtualAddressSpace().pageCount(),
        64ULL
    );

    EXPECT_NE(
        first.processId(),
        second.processId()
    );
}

TEST(ProcessIntegrationTest, ProcessIdsCanIdentifyMemoryAccessOwners)
{
    ProcessManager manager;

    Process& first = manager.createProcess(
        16,
        4096
    );

    Process& second = manager.createProcess(
        16,
        4096
    );

    const memory::access::MemoryAccess firstAccess{
        first.processId(),
        memory::access::VirtualAddress{0},
        memory::access::MemoryAccessOperation::Read,
        memory::access::AccessSequenceNumber{1}
    };

    const memory::access::MemoryAccess secondAccess{
        second.processId(),
        memory::access::VirtualAddress{0},
        memory::access::MemoryAccessOperation::Read,
        memory::access::AccessSequenceNumber{2}
    };

    EXPECT_NE(
        firstAccess.processId(),
        secondAccess.processId()
    );

    EXPECT_EQ(
        firstAccess.processId(),
        first.processId()
    );

    EXPECT_EQ(
        secondAccess.processId(),
        second.processId()
    );
}

} // namespace emmus::simulation