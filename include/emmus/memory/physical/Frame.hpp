#pragma once

#include <optional>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::memory::physical
{

/**
 * @brief Error conditions for invalid Frame state transitions.
 */
enum class FrameError
{
    AlreadyOccupied,
    AlreadyFree
};


/**
 * @brief Represents one physical-memory frame.
 *
 * A Frame represents the identity and current page association of a single
 * physical-memory frame.
 *
 * Frame management responsibilities such as allocation, release, collection,
 * and replacement belong to PhysicalMemoryManager.
 */
class Frame
{
public:

    using FrameId = identifiers::FrameId;
    using PageId = identifiers::PageId;


    /**
     * @brief Constructs a free frame with the supplied identity.
     *
     * @param id Unique identifier of the physical-memory frame.
     */
    explicit Frame(
        FrameId id
    ) noexcept;


    Frame(const Frame&) = default;

    Frame(Frame&&) noexcept = default;

    Frame& operator=(const Frame&) = default;

    Frame& operator=(Frame&&) noexcept = default;

    ~Frame() = default;


    /**
     * @brief Returns the frame's identity.
     */
    [[nodiscard]]
    FrameId id() const noexcept;


    /**
     * @brief Returns whether a page is currently mapped to this frame.
     */
    [[nodiscard]]
    bool isOccupied() const noexcept;


    /**
     * @brief Returns the page currently mapped to this frame.
     *
     * @return The mapped PageId, or std::nullopt when the frame is free.
     */
    [[nodiscard]]
    const std::optional<PageId>& mappedPage() const noexcept;


    /**
     * @brief Maps a page into this frame.
     *
     * A page can only be mapped when the frame is currently free.
     *
     * @return std::nullopt on success.
     *
     * @return FrameError::AlreadyOccupied when the frame is already
     *         occupied. The existing mapping remains unchanged.
     */
    [[nodiscard]]
    std::optional<FrameError> mapPage(
        PageId pageId
    ) noexcept;


    /**
     * @brief Removes the page currently mapped to this frame.
     *
     * @return std::nullopt on success.
     *
     * @return FrameError::AlreadyFree when the frame is already free.
     */
    [[nodiscard]]
    std::optional<FrameError> unmapPage() noexcept;


private:

    FrameId id_;

    std::optional<PageId> mappedPage_;
};

} // namespace emmus::memory::physical
