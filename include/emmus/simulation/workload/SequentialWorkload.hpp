#pragma once

#include <cstddef>
#include <vector>

#include "emmus/simulation/workload/IWorkload.hpp"

namespace emmus::simulation::workload
{

/**
 * @brief Generates a deterministic sequential memory-access workload.
 *
 * The workload walks a contiguous range of virtual pages and repeats that
 * range until the configured number of accesses has been generated.
 *
 * For example:
 *
 *     startingPage = 2
 *     patternLength = 3
 *     accessCount = 8
 *
 * generates:
 *
 *     page 2, page 3, page 4,
 *     page 2, page 3, page 4,
 *     page 2, page 3
 *
 * Every generated access starts at offset zero within its page.
 *
 * The workload is deterministic because no random state is involved.
 */
class SequentialWorkload final : public IWorkload
{
public:

    using ProcessId =
        memory::identifiers::ProcessId;

    using PageId =
        memory::identifiers::PageId;

    using PageSize =
        memory::access::PageSize;


    /**
     * @brief Constructs a sequential workload using the entire address space
     *        as its repeating pattern.
     *
     * This constructor preserves the original SequentialWorkload API.
     *
     * The generated sequence starts at page zero and uses pageCount as the
     * pattern length.
     *
     * @param processId Process that owns the generated accesses.
     * @param pageCount Number of pages in the process address space.
     * @param pageSize Size of each virtual page in bytes.
     * @param accessCount Number of memory accesses to generate.
     */
    SequentialWorkload(
        ProcessId processId,
        std::size_t pageCount,
        PageSize pageSize,
        std::size_t accessCount
    );


    /**
     * @brief Constructs a configurable sequential workload.
     *
     * The generated page sequence is:
     *
     *     startingPage,
     *     startingPage + 1,
     *     ...
     *     startingPage + patternLength - 1
     *
     * and then repeats from startingPage until accessCount accesses have
     * been generated.
     *
     * @param processId Process that owns the generated accesses.
     * @param pageCount Number of pages in the process address space.
     * @param pageSize Size of each virtual page in bytes.
     * @param accessCount Number of memory accesses to generate.
     * @param startingPage First page in the sequential pattern.
     * @param patternLength Number of contiguous pages in the repeating
     *        sequential pattern.
     *
     * @throws std::invalid_argument when:
     *         - pageCount is zero;
     *         - accessCount is zero;
     *         - patternLength is zero;
     *         - startingPage is outside the address space;
     *         - patternLength extends beyond the address space;
     *         - a generated virtual address would overflow uint64_t.
     */
    SequentialWorkload(
        ProcessId processId,
        std::size_t pageCount,
        PageSize pageSize,
        std::size_t accessCount,
        PageId startingPage,
        std::size_t patternLength
    );


    [[nodiscard]]
    bool hasNext() const noexcept override;


    memory::access::MemoryAccess nextAccess() override;


    void reset() override;


    [[nodiscard]]
    std::size_t size() const noexcept override;


private:

    std::vector<memory::access::MemoryAccess> accesses_;

    std::size_t nextIndex_{0};
};

} // namespace emmus::simulation::workload