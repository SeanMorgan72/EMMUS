#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::memory::physical
{

enum class FrameState
{
    Free,
    Allocated
};

struct FrameUtilizationSnapshot
{
    using PageId = emmus::memory::identifiers::PageId;
    using FrameId = emmus::memory::identifiers::FrameId;

    explicit FrameUtilizationSnapshot(FrameId id) noexcept
        : frameId(id)
    {
    }

    FrameId frameId;
    FrameState state{FrameState::Free};
    bool isAllocated{false};
    bool isOccupied{false};
    std::optional<PageId> pageId;
};

struct PhysicalMemoryUtilization
{
    using FrameId = emmus::memory::identifiers::FrameId;

    std::size_t totalFrames{0U};
    std::size_t allocatedFrames{0U};
    std::size_t freeFrames{0U};
    double utilizationRatio{0.0};
    double utilizationPercent{0.0};
    std::vector<FrameUtilizationSnapshot> frames;

    [[nodiscard]]
    bool isEmpty() const noexcept
    {
        return totalFrames == 0U || allocatedFrames == 0U;
    }

    [[nodiscard]]
    bool isFullyUtilized() const noexcept
    {
        return totalFrames > 0U && freeFrames == 0U;
    }

    [[nodiscard]]
    std::size_t allocatedCount() const noexcept
    {
        return allocatedFrames;
    }

    [[nodiscard]]
    std::size_t freeCount() const noexcept
    {
        return freeFrames;
    }

    [[nodiscard]]
    double utilizationRatioFor(FrameId frameId) const noexcept
    {
        if (frames.empty())
        {
            return 0.0;
        }

        const auto index = static_cast<std::size_t>(frameId.value());
        if (index >= frames.size())
        {
            return 0.0;
        }

        return frames[index].isAllocated ? 1.0 : 0.0;
    }
};

} // namespace emmus::memory::physical
