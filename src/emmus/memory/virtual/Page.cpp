#include "emmus/memory/virtual/Page.hpp"

namespace emmus::memory::virtual_memory
{

Page::Page(
    PageId pageId,
    ProcessId processId
) noexcept
    : pageId_(pageId)
    , processId_(processId)
    , mappedFrame_(std::nullopt)
    , dirty_(false)
    , referenced_(false)
{
}


Page::PageId Page::id() const noexcept
{
    return pageId_;
}


Page::ProcessId Page::processId() const noexcept
{
    return processId_;
}


bool Page::isResident() const noexcept
{
    return mappedFrame_.has_value();
}


const std::optional<Page::FrameId>& Page::mappedFrame() const noexcept
{
    return mappedFrame_;
}


bool Page::isDirty() const noexcept
{
    return dirty_;
}


bool Page::isReferenced() const noexcept
{
    return referenced_;
}


std::optional<PageError> Page::mapToFrame(
    FrameId frameId
) noexcept
{
    if (isResident())
    {
        return PageError::AlreadyResident;
    }

    mappedFrame_ = frameId;

    return std::nullopt;
}


std::optional<PageError> Page::unmapFromFrame() noexcept
{
    if (!isResident())
    {
        return PageError::AlreadyNonResident;
    }

    mappedFrame_.reset();
    dirty_ = false;
    referenced_ = false;

    return std::nullopt;
}


void Page::markDirty() noexcept
{
    dirty_ = true;
}


void Page::clearDirty() noexcept
{
    dirty_ = false;
}


void Page::markReferenced() noexcept
{
    referenced_ = true;
}


void Page::clearReferenced() noexcept
{
    referenced_ = false;
}

} // namespace emmus::memory::virtual_memory