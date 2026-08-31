#include "emmus/memory/physical/PhysicalMemoryManager.hpp"

namespace emmus::memory::physical
{

PhysicalMemoryManager::PhysicalMemoryManager(
    FrameCount frameCount
)
{
    frames_.reserve(frameCount);

    for (FrameCount index = 0; index < frameCount; ++index)
    {
        frames_.emplace_back(
            FrameId{
                static_cast<FrameId::ValueType>(index)
            }
        );
    }
}


PhysicalMemoryManager::FrameCount
PhysicalMemoryManager::capacity() const noexcept
{
    return frames_.size();
}


PhysicalMemoryManager::FrameCount
PhysicalMemoryManager::freeFrameCount() const noexcept
{
    return capacity() - allocatedFrameCount_;
}


PhysicalMemoryManager::FrameCount
PhysicalMemoryManager::allocatedFrameCount() const noexcept
{
    return allocatedFrameCount_;
}


bool PhysicalMemoryManager::hasFreeFrame() const noexcept
{
    return allocatedFrameCount_ < capacity();
}


bool PhysicalMemoryManager::isValidFrameId(
    FrameId frameId
) const noexcept
{
    return frameId.value() < frames_.size();
}


bool PhysicalMemoryManager::isFrameFree(
    FrameId frameId
) const noexcept
{
    const Frame* selectedFrame = frame(frameId);

    if (selectedFrame == nullptr)
    {
        return false;
    }

    return !selectedFrame->isOccupied();
}


bool PhysicalMemoryManager::isPageMapped(
    PageId pageId
) const noexcept
{
    return frameForPage(pageId).has_value();
}


std::optional<PhysicalMemoryManager::FrameId>
PhysicalMemoryManager::allocateFrame(
    PageId pageId
) noexcept
{
    /*
     * A page may have only one physical-frame allocation.
     *
     * Reject duplicate allocation before modifying any frame
     * or allocation counters.
     */
    if (isPageMapped(pageId))
    {
        return std::nullopt;
    }


    /*
     * Find the first free frame.
     *
     * frames_ is maintained in FrameId order, so this produces
     * deterministic lowest-numbered-free-frame allocation.
     */
    for (Frame& candidate : frames_)
    {
        if (candidate.isOccupied())
        {
            continue;
        }


        /*
         * Frame owns the actual page-mapping state transition.
         */
        const auto result =
            candidate.mapPage(pageId);


        /*
         * The frame was verified free immediately before mapPage().
         * Therefore, success is expected here.
         */
        if (!result.has_value())
        {
            ++allocatedFrameCount_;

            return candidate.id();
        }
    }


    /*
     * All physical frames are occupied.
     *
     * PhysicalMemoryManager does not select a victim or perform
     * page replacement. The caller/replacement subsystem must
     * handle that responsibility.
     */
    return std::nullopt;
}


bool PhysicalMemoryManager::releaseFrame(
    FrameId frameId
) noexcept
{
    Frame* selectedFrame = frame(frameId);

    /*
     * Invalid FrameId.
     */
    if (selectedFrame == nullptr)
    {
        return false;
    }


    /*
     * Already-free frame.
     */
    if (!selectedFrame->isOccupied())
    {
        return false;
    }


    /*
     * Frame owns the actual page-unmapping state transition.
     */
    const auto result =
        selectedFrame->unmapPage();


    if (result.has_value())
    {
        return false;
    }


    --allocatedFrameCount_;

    return true;
}


std::optional<PhysicalMemoryManager::FrameId>
PhysicalMemoryManager::frameForPage(
    PageId pageId
) const noexcept
{
    for (const Frame& candidate : frames_)
    {
        const auto& mappedPage =
            candidate.mappedPage();


        if (mappedPage.has_value() &&
            mappedPage.value() == pageId)
        {
            return candidate.id();
        }
    }


    return std::nullopt;
}


const Frame*
PhysicalMemoryManager::frame(
    FrameId frameId
) const noexcept
{
    if (!isValidFrameId(frameId))
    {
        return nullptr;
    }

    return &frames_[frameIndex(frameId)];
}


Frame*
PhysicalMemoryManager::frame(
    FrameId frameId
) noexcept
{
    if (!isValidFrameId(frameId))
    {
        return nullptr;
    }

    return &frames_[frameIndex(frameId)];
}


std::size_t
PhysicalMemoryManager::frameIndex(
    FrameId frameId
) const noexcept
{
    return static_cast<std::size_t>(
        frameId.value()
    );
}

} // namespace emmus::memory::physical