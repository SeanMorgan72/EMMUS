#include "emmus/memory/mmu/PageTable.hpp"

namespace emmus::memory::mmu
{

bool PageTable::map(
    const PageId pageId,
    const FrameId frameId
)
{
    const auto [iterator, inserted] =
        mappings_.emplace(pageId, frameId);

    static_cast<void>(iterator);

    return inserted;
}


bool PageTable::updateMapping(
    const PageId pageId,
    const FrameId frameId
)
{
    const auto iterator = mappings_.find(pageId);

    if (iterator == mappings_.end())
    {
        return false;
    }

    iterator->second = frameId;

    return true;
}


std::optional<PageTable::FrameId> PageTable::lookup(
    const PageId pageId
) const noexcept
{
    const auto iterator = mappings_.find(pageId);

    if (iterator == mappings_.end())
    {
        return std::nullopt;
    }

    return iterator->second;
}


bool PageTable::isMapped(
    const PageId pageId
) const noexcept
{
    return mappings_.contains(pageId);
}


bool PageTable::unmap(
    const PageId pageId
) noexcept
{
    return mappings_.erase(pageId) != 0U;
}


std::size_t PageTable::size() const noexcept
{
    return mappings_.size();
}


bool PageTable::empty() const noexcept
{
    return mappings_.empty();
}

} // namespace emmus::memory::mmu