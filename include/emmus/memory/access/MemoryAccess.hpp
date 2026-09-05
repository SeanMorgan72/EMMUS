#pragma once


#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::memory::access {

/**
 * @brief Identifies the operation requested by a memory access.
 *
 * Read operations do not modify page contents.
 * Write operations modify page contents and therefore make the
 * corresponding resident page dirty when the access is processed.
 */
enum class MemoryAccessOperation {
    Read,
    Write
};

/**
 * @brief Represents one virtual-memory access request.
 *
 * A MemoryAccess identifies:
 *   - the process issuing the access,
 *   - the virtual address being accessed,
 *   - the requested operation,
 *   - the access sequence number.
 *
 * This type represents the request only. It does not perform address
 * translation, page-fault handling, physical-memory allocation, or
 * page-replacement decisions.
 */
class MemoryAccess {
public:
    using ProcessId = emmus::memory::identifiers::ProcessId;

    /**
     * @brief Constructs a memory-access request.
     *
     * @param processId Process issuing the access.
     * @param virtualAddress Virtual address being accessed.
     * @param operation Requested memory operation.
     * @param sequenceNumber Deterministic access sequence number.
     */
    constexpr MemoryAccess(
        ProcessId processId,
        VirtualAddress virtualAddress,
        MemoryAccessOperation operation,
        AccessSequenceNumber sequenceNumber) noexcept
        : processId_(processId),
          virtualAddress_(virtualAddress),
          operation_(operation),
          sequenceNumber_(sequenceNumber)
    {
    }

    /**
     * @brief Returns the process issuing the access.
     */
    [[nodiscard]] constexpr ProcessId processId() const noexcept
    {
        return processId_;
    }

    /**
     * @brief Returns the virtual address being accessed.
     */
    [[nodiscard]] constexpr VirtualAddress virtualAddress() const noexcept
    {
        return virtualAddress_;
    }

    /**
     * @brief Returns the requested memory operation.
     */
    [[nodiscard]] constexpr MemoryAccessOperation operation() const noexcept
    {
        return operation_;
    }

    /**
     * @brief Returns the deterministic access sequence number.
     */
    [[nodiscard]] constexpr AccessSequenceNumber sequenceNumber() const noexcept
    {
        return sequenceNumber_;
    }

    /**
     * @brief Returns whether this access is a read.
     */
    [[nodiscard]] constexpr bool isRead() const noexcept
    {
        return operation_ == MemoryAccessOperation::Read;
    }

    /**
     * @brief Returns whether this access is a write.
     */
    [[nodiscard]] constexpr bool isWrite() const noexcept
    {
        return operation_ == MemoryAccessOperation::Write;
    }

    /**
     * @brief Compares two memory-access requests for equality.
     */
    friend constexpr bool operator==(
        const MemoryAccess&,
        const MemoryAccess&) noexcept = default;

private:
    ProcessId processId_;
    VirtualAddress virtualAddress_;
    MemoryAccessOperation operation_;
    AccessSequenceNumber sequenceNumber_;
};

}  // namespace emmus::memory::access