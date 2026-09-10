#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/application/MemoryAccessExecutor.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/mmu/MemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual_memory/Page.hpp"

namespace emmus::test
{

class MemoryAccessExecutorTest : public ::testing::Test
{
protected:

    using MemoryAccess =
        memory::access::MemoryAccess;

    using MemoryAccessOperation =
        memory::access::MemoryAccessOperation;

    using VirtualAddress =
        memory::access::VirtualAddress;

    using PageSize =
        memory::access::PageSize;

    using AccessSequenceNumber =
        memory::access::AccessSequenceNumber;

    using PageId =
        memory::identifiers::PageId;

    using ProcessId =
        memory::identifiers::ProcessId;

    using Page =
        memory::virtual_memory::Page;

    using PageTable =
        memory::mmu::PageTable;

    using PhysicalMemoryManager =
        memory::physical::PhysicalMemoryManager;

    using FIFOPageReplacementPolicy =
        algorithms::replacement::FIFOPageReplacementPolicy;

    using MemoryManagementUnit =
        memory::mmu::MemoryManagementUnit;

    using MemoryAccessExecutor =
        application::MemoryAccessExecutor;

    static constexpr std::size_t kFrameCount = 2U;
    static constexpr std::size_t kPageSize = 4096U;

    void SetUp() override
    {
        pageTable_ =
            std::make_unique<PageTable>();

        physicalMemory_ =
            std::make_unique<PhysicalMemoryManager>(
                kFrameCount
            );

        replacementPolicy_ =
            std::make_unique<FIFOPageReplacementPolicy>();

        mmu_ =
            std::make_unique<MemoryManagementUnit>(
                *pageTable_,
                *physicalMemory_,
                *replacementPolicy_,
                PageSize{kPageSize}
            );

        executor_ =
            std::make_unique<MemoryAccessExecutor>(
                *mmu_
            );
    }

    void registerPage(
        std::uint64_t pageId,
        std::uint64_t processId
    )
    {
        ASSERT_TRUE(
            mmu_->registerPage(
                Page{
                    PageId{pageId},
                    ProcessId{processId}
                }
            )
        );
    }

    std::unique_ptr<PageTable> pageTable_;
    std::unique_ptr<PhysicalMemoryManager> physicalMemory_;
    std::unique_ptr<FIFOPageReplacementPolicy> replacementPolicy_;
    std::unique_ptr<MemoryManagementUnit> mmu_;
    std::unique_ptr<MemoryAccessExecutor> executor_;
};


TEST_F(
    MemoryAccessExecutorTest,
    ExecutesSingleAccessSuccessfully
)
{
    registerPage(0U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 1U);

    const auto& record =
        *executionResult.begin();

    EXPECT_TRUE(record.result.success());
    EXPECT_TRUE(record.result.pageFault());
    EXPECT_FALSE(record.result.pageReplacement());

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().pageReplacementCount(),
        0U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    ExecutesMultipleAccessesInInputOrder
)
{
    registerPage(0U, 1U);
    registerPage(1U, 1U);
    registerPage(2U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{2U * kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{2U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 3U);

    auto iterator = executionResult.begin();

    EXPECT_EQ(
        iterator->access.sequenceNumber().value(),
        0U
    );

    ++iterator;

    EXPECT_EQ(
        iterator->access.sequenceNumber().value(),
        1U
    );

    ++iterator;

    EXPECT_EQ(
        iterator->access.sequenceNumber().value(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        3U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    RecordsPageHitAfterInitialFault
)
{
    registerPage(0U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{128U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 2U);

    auto iterator = executionResult.begin();

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());

    ++iterator;

    EXPECT_TRUE(iterator->result.success());
    EXPECT_FALSE(iterator->result.pageFault());

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        1U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    SupportsMultipleProcesses
)
{
    registerPage(0U, 1U);
    registerPage(1U, 2U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{2U},
            VirtualAddress{kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 2U);

    auto iterator = executionResult.begin();

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());

    ++iterator;

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        2U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    RejectsInvalidProcessReference
)
{
    registerPage(0U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{999U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 1U);

    const auto& result =
        executionResult.begin()->result;

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.pageFault());
    EXPECT_FALSE(result.pageReplacement());

    EXPECT_FALSE(result.frameId().has_value());
    EXPECT_FALSE(result.physicalAddress().has_value());

    EXPECT_FALSE(
        result.errorInformation().empty()
    );

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        0U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    ContinuesAfterInvalidAccess
)
{
    registerPage(0U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{999U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 2U);

    auto iterator = executionResult.begin();

    EXPECT_FALSE(iterator->result.success());

    ++iterator;

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        1U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    AllocatesDifferentFramesForDifferentResidentPages
)
{
    registerPage(0U, 1U);
    registerPage(1U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 2U);

    auto iterator = executionResult.begin();

    ASSERT_TRUE(
        iterator->result.frameId().has_value()
    );

    const auto firstFrame =
        iterator->result.frameId().value();

    ++iterator;

    ASSERT_TRUE(
        iterator->result.frameId().has_value()
    );

    const auto secondFrame =
        iterator->result.frameId().value();

    EXPECT_NE(firstFrame, secondFrame);

    EXPECT_EQ(
        physicalMemory_->allocatedFrameCount(),
        2U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    PerformsReplacementWhenFramesAreExhausted
)
{
    registerPage(0U, 1U);
    registerPage(1U, 1U);
    registerPage(2U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{2U * kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{2U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 3U);

    auto iterator = executionResult.begin();

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());
    EXPECT_FALSE(iterator->result.pageReplacement());

    ++iterator;

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());
    EXPECT_FALSE(iterator->result.pageReplacement());

    ++iterator;

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());
    EXPECT_TRUE(iterator->result.pageReplacement());

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        3U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        3U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        executionResult.statistics().pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        physicalMemory_->allocatedFrameCount(),
        2U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    RecordsDirtyEvictionForWrittenPage
)
{
    registerPage(0U, 1U);
    registerPage(1U, 1U);
    registerPage(2U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Write,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{2U * kPageSize},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{2U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    ASSERT_EQ(executionResult.size(), 3U);

    auto iterator = executionResult.begin();

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());

    ++iterator;

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());

    ++iterator;

    EXPECT_TRUE(iterator->result.success());
    EXPECT_TRUE(iterator->result.pageFault());
    EXPECT_TRUE(iterator->result.pageReplacement());
    EXPECT_TRUE(iterator->result.dirtyEviction());

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        3U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        3U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        3U
    );

    EXPECT_EQ(
        executionResult.statistics().pageReplacementCount(),
        1U
    );

    EXPECT_EQ(
        executionResult.statistics().dirtyEvictionCount(),
        1U
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    EmptyAccessSequenceProducesEmptyResult
)
{
    const std::vector<MemoryAccess> accesses;

    const auto executionResult =
        executor_->execute(accesses);

    EXPECT_TRUE(executionResult.empty());
    EXPECT_EQ(executionResult.size(), 0U);

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageReplacementCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().dirtyEvictionCount(),
        0U
    );

    EXPECT_DOUBLE_EQ(
        executionResult.statistics().pageFaultRate(),
        0.0
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    CalculatesPageFaultRate
)
{
    registerPage(0U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        },
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{64U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{1U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    EXPECT_EQ(
        executionResult.statistics().memoryAccessCount(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().successfulAccessCount(),
        2U
    );

    EXPECT_EQ(
        executionResult.statistics().failedAccessCount(),
        0U
    );

    EXPECT_EQ(
        executionResult.statistics().pageFaultCount(),
        1U
    );

    EXPECT_DOUBLE_EQ(
        executionResult.statistics().pageFaultRate(),
        0.5
    );
}


TEST_F(
    MemoryAccessExecutorTest,
    RecordsExecutionTime
)
{
    registerPage(0U, 1U);

    const std::vector<MemoryAccess> accesses{
        MemoryAccess{
            ProcessId{1U},
            VirtualAddress{0U},
            MemoryAccessOperation::Read,
            AccessSequenceNumber{0U}
        }
    };

    const auto executionResult =
        executor_->execute(accesses);

    EXPECT_GE(
        executionResult.statistics().totalExecutionTime().count(),
        0
    );
}

} // namespace emmus::test