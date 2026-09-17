#include "emmus/application/MemoryAccessExecutor.hpp"

#include <chrono>
#include <iostream>

namespace emmus::application
{

MemoryAccessExecutor::MemoryAccessExecutor(
    MemoryManagementUnit& memoryManagementUnit
) noexcept
    : memoryManagementUnit_(memoryManagementUnit)
{
}


MemoryAccessExecutionResult
MemoryAccessExecutor::execute(
    std::span<const Access> accesses
)
{
    MemoryAccessExecutionResult executionResult;

    executionResult.records_.reserve(
        accesses.size()
    );

    const auto startTime =
        std::chrono::steady_clock::now();

    for (const Access& access : accesses)
    {
        const auto result =
            memoryManagementUnit_.access(access);
        
        std::cerr
        << "access: success=" << result.success()
        << " pageFault=" << result.pageFault()
        << " replacement=" << result.pageReplacement()
        << " error=\"" << result.errorInformation() << "\"\n";

        executionResult.records_.push_back(
            MemoryAccessExecutionRecord{
                access,
                result
            }
        );

        auto& statistics =
            executionResult.statistics_;

        ++statistics.memoryAccessCount_;

        if (result.success())
        {
            ++statistics.successfulAccessCount_;
        }
        else
        {
            ++statistics.failedAccessCount_;
        }

        if (result.pageFault())
        {
            ++statistics.pageFaultCount_;
        }

        if (result.pageReplacement())
        {
            ++statistics.pageReplacementCount_;
        }

        if (result.dirtyEviction())
        {
            ++statistics.dirtyEvictionCount_;
        }
    }

    const auto endTime =
        std::chrono::steady_clock::now();

    executionResult.statistics_.totalExecutionTime_ =
        std::chrono::duration_cast<
            MemoryAccessExecutionStatistics::Duration
        >(
            endTime - startTime
        );

    return executionResult;
}


MemoryAccessExecutionResult
MemoryAccessExecutor::execute(
    const std::vector<Access>& accesses
)
{
    return execute(
        std::span<const Access>{
            accesses.data(),
            accesses.size()
        }
    );
}

} // namespace emmus::application