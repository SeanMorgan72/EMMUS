#pragma once

#include <optional>
#include <string>
#include <utility>

#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::memory::access {

class MemoryAccessResult {
public:
    using FrameId = emmus::memory::identifiers::FrameId;

    constexpr MemoryAccessResult(
        bool success,
        bool pageFault,
        bool pageReplacement,
        std::optional<FrameId> frameId,
        std::optional<PhysicalAddress> physicalAddress,
        bool dirtyEviction,
        std::string errorInformation = {})
        : success_(success),
          pageFault_(pageFault),
          pageReplacement_(pageReplacement),
          frameId_(frameId),
          physicalAddress_(physicalAddress),
          dirtyEviction_(dirtyEviction),
          errorInformation_(std::move(errorInformation))
    {
    }

    [[nodiscard]] constexpr bool success() const noexcept
    {
        return success_;
    }

    [[nodiscard]] constexpr bool pageFault() const noexcept
    {
        return pageFault_;
    }

    [[nodiscard]] constexpr bool pageReplacement() const noexcept
    {
        return pageReplacement_;
    }

    [[nodiscard]] constexpr const std::optional<FrameId>& frameId() const noexcept
    {
        return frameId_;
    }

    [[nodiscard]] constexpr const std::optional<PhysicalAddress>& physicalAddress() const noexcept
    {
        return physicalAddress_;
    }

    [[nodiscard]] constexpr bool dirtyEviction() const noexcept
    {
        return dirtyEviction_;
    }

    [[nodiscard]] const std::string& errorInformation() const noexcept
    {
        return errorInformation_;
    }

private:
    bool success_;
    bool pageFault_;
    bool pageReplacement_;
    std::optional<FrameId> frameId_;
    std::optional<PhysicalAddress> physicalAddress_;
    bool dirtyEviction_;
    std::string errorInformation_;
};

} // namespace emmus::memory::access