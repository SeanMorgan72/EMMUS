#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/physical/Frame.hpp"

namespace emmus::memory::physical
{

/**
 * @brief Manages physical-memory frames and page-to-frame allocations.
 *
 * PhysicalMemoryManager owns the collection of physical-memory frames
 * and is responsible for allocating and releasing frames.
 *
 * This component does not perform:
 *
 * - page replacement;
 * - victim selection;
 * - virtual-to-physical address translation;
 * - page-table management;
 * - MMU access processing.
 */
class PhysicalMemoryManager
{
public:

    using FrameId = identifiers::FrameId;
    using PageId = identifiers::PageId;
    using FrameCount = std::size_t;


    /**
     * @brief Constructs physical memory containing frameCount frames.
     *
     * All frames are initially free.
     *
     * Frame identifiers are assigned sequentially beginning at zero.
     *
     * @param frameCount Number of physical frames.
     */
    explicit PhysicalMemoryManager(
        FrameCount frameCount
    );


    PhysicalMemoryManager(
        const PhysicalMemoryManager&
    ) = delete;

    PhysicalMemoryManager& operator=(
        const PhysicalMemoryManager&
    ) = delete;

    PhysicalMemoryManager(
        PhysicalMemoryManager&&
    ) noexcept = default;

    PhysicalMemoryManager& operator=(
        PhysicalMemoryManager&&
    ) noexcept = default;

    ~PhysicalMemoryManager() = default;


    /**
     * @brief Returns the total number of physical frames.
     */
    [[nodiscard]]
    FrameCount capacity() const noexcept;


    /**
     * @brief Returns the number of currently free frames.
     */
    [[nodiscard]]
    FrameCount freeFrameCount() const noexcept;


    /**
     * @brief Returns the number of currently allocated frames.
     */
    [[nodiscard]]
    FrameCount allocatedFrameCount() const noexcept;


    /**
     * @brief Determines whether a free physical frame exists.
     */
    [[nodiscard]]
    bool hasFreeFrame() const noexcept;


    /**
     * @brief Determines whether a FrameId is valid.
     *
     * @param frameId Frame identifier to validate.
     *
     * @return true when frameId identifies a managed frame.
     */
    [[nodiscard]]
    bool isValidFrameId(
        FrameId frameId
    ) const noexcept;


    /**
     * @brief Determines whether a frame is currently free.
     *
     * Invalid FrameIds return false.
     *
     * @param frameId Frame identifier to inspect.
     */
    [[nodiscard]]
    bool isFrameFree(
        FrameId frameId
    ) const noexcept;


    /**
     * @brief Determines whether a page is currently mapped to a frame.
     *
     * @param pageId Page identifier to search for.
     */
    [[nodiscard]]
    bool isPageMapped(
        PageId pageId
    ) const noexcept;


    /**
     * @brief Allocates a free frame for a page.
     *
     * The first available frame is selected. Since the internal
     * frame collection is maintained in FrameId order, allocation
     * is deterministic and selects the lowest-numbered free frame.
     *
     * A page that is already mapped cannot be allocated again.
     *
     * No page-replacement decision is made when physical memory
     * is exhausted.
     *
     * @param pageId Page to associate with a physical frame.
     *
     * @return The allocated FrameId on success.
     *
     * @return std::nullopt when the page is already mapped or
     *         no free frame is available.
     */
    [[nodiscard]]
    std::optional<FrameId> allocateFrame(
        PageId pageId
    ) noexcept;


    /**
     * @brief Releases an occupied physical frame.
     *
     * The frame's page mapping is removed and the frame becomes free.
     *
     * @param frameId Frame to release.
     *
     * @return true when the frame was successfully released.
     *
     * @return false when frameId is invalid or the frame is already free.
     */
    bool releaseFrame(
        FrameId frameId
    ) noexcept;


    /**
     * @brief Finds the frame containing a specified page.
     *
     * @param pageId Page identifier to search for.
     *
     * @return The FrameId containing the page.
     *
     * @return std::nullopt when the page is not currently mapped.
     */
    [[nodiscard]]
    std::optional<FrameId> frameForPage(
        PageId pageId
    ) const noexcept;


    /**
     * @brief Provides read-only access to a managed frame.
     *
     * @param frameId Frame identifier.
     *
     * @return Pointer to the frame when valid.
     *
     * @return nullptr when frameId is invalid.
     */
    [[nodiscard]]
    const Frame* frame(
        FrameId frameId
    ) const noexcept;


    /**
     * @brief Provides mutable access to a managed frame.
     *
     * @param frameId Frame identifier.
     *
     * @return Pointer to the frame when valid.
     *
     * @return nullptr when frameId is invalid.
     */
    [[nodiscard]]
    Frame* frame(
        FrameId frameId
    ) noexcept;


private:

    /**
     * @brief Converts a valid FrameId to a vector index.
     */
    [[nodiscard]]
    std::size_t frameIndex(
        FrameId frameId
    ) const noexcept;


    std::vector<Frame> frames_;

    FrameCount allocatedFrameCount_{0};
};

} // namespace emmus::memory::physical