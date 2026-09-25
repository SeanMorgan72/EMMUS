#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::simulation::activity
{

enum class SimulationActivityType
{
    Access,
    PageFault,
    PageReplacement,
    FrameAssignment,
    DirtyEviction,
    Status
};

struct SimulationActivityEvent
{
    using ProcessId = emmus::memory::identifiers::ProcessId;
    using PageId = emmus::memory::identifiers::PageId;
    using FrameId = emmus::memory::identifiers::FrameId;
    using MemoryAccessOperation =
        emmus::memory::access::MemoryAccessOperation;

    std::size_t sequence{0U};
    SimulationActivityType type{SimulationActivityType::Status};
    std::optional<ProcessId> processId;
    std::optional<PageId> pageId;
    std::optional<PageId> victimPageId;
    std::optional<FrameId> frameId;
    std::optional<FrameId> victimFrameId;
    std::optional<FrameId> replacementFrameId;
    std::optional<std::uint64_t> virtualPageNumber;
    std::optional<MemoryAccessOperation> operation;
    std::optional<bool> success;
    bool pageFault{false};
    bool pageReplacement{false};
    bool dirtyEviction{false};
    std::string summary;
    std::string detail;

    [[nodiscard]] std::string render() const;

    [[nodiscard]] constexpr bool operator==(
        const SimulationActivityEvent&) const = default;
};

class SimulationActivityLog final
{
public:
    SimulationActivityLog() = default;

    void record(const SimulationActivityEvent& event);

    void recordAccess(
        emmus::memory::identifiers::ProcessId processId,
        emmus::memory::identifiers::PageId pageId,
        std::optional<emmus::memory::identifiers::FrameId> frameId,
        emmus::memory::access::MemoryAccessOperation operation,
        bool success,
        bool pageFault,
        bool pageReplacement,
        bool dirtyEviction,
        std::string detail = {});

    void recordPageFault(
        emmus::memory::identifiers::ProcessId processId,
        emmus::memory::identifiers::PageId pageId,
        std::optional<emmus::memory::identifiers::FrameId> frameId,
        std::string detail = {});

    void recordPageReplacement(
        emmus::memory::identifiers::ProcessId processId,
        emmus::memory::identifiers::PageId pageId,
        emmus::memory::identifiers::FrameId victimFrameId,
        emmus::memory::identifiers::PageId victimPageId,
        std::string detail = {});

    void recordFrameAssignment(
        emmus::memory::identifiers::ProcessId processId,
        emmus::memory::identifiers::PageId pageId,
        emmus::memory::identifiers::FrameId frameId,
        std::string detail = {});

    void recordDirtyEviction(
        emmus::memory::identifiers::ProcessId processId,
        emmus::memory::identifiers::PageId pageId,
        emmus::memory::identifiers::FrameId frameId,
        std::string detail = {});

    void recordStatus(
        emmus::memory::identifiers::ProcessId processId,
        std::string detail,
        std::optional<emmus::memory::identifiers::PageId> pageId = std::nullopt,
        std::optional<emmus::memory::identifiers::FrameId> frameId = std::nullopt,
        bool success = true);

    void clear() noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] const std::vector<SimulationActivityEvent>& events() const noexcept;

    [[nodiscard]] std::string render() const;

    [[nodiscard]] std::string renderCompact() const;

    [[nodiscard]] constexpr bool operator==(
        const SimulationActivityLog&) const = default;

private:
    std::vector<SimulationActivityEvent> events_{};
    std::size_t nextSequence_{0U};
};

} // namespace emmus::simulation::activity
