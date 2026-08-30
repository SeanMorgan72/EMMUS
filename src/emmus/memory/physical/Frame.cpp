#include "emmus/memory/physical/Frame.hpp"

namespace emmus::memory::physical
{

Frame::Frame(FrameId id) noexcept
    : id_(id)
{
}

Frame::FrameId Frame::id() const noexcept
{
    return id_;
}

bool Frame::isOccupied() const noexcept
{
    return mappedPage_.has_value();
}

const std::optional<Frame::PageId>& Frame::mappedPage() const noexcept
{
    return mappedPage_;
}

std::optional<FrameError> Frame::mapPage(PageId pageId) noexcept
{
    if (isOccupied())
    {
        return FrameError::AlreadyOccupied;
    }

    mappedPage_ = pageId;

    return std::nullopt;
}

std::optional<FrameError> Frame::unmapPage() noexcept
{
    if (!isOccupied())
    {
        return FrameError::AlreadyFree;
    }

    mappedPage_.reset();

    return std::nullopt;
}

} // namespace emmus::memory::physical