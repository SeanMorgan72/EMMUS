#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::memory::mmu
{

/**
 * @brief Maintains mappings between virtual pages and physical frames.
 *
 * PageTable is responsible only for maintaining the relationship between
 * virtual PageIds and physical FrameIds. It does not allocate frames,
 * manage physical memory, perform address translation, handle page faults,
 * or select page-replacement victims.
 */
class PageTable
{
public:
    using PageId = emmus::memory::identifiers::PageId;
    using FrameId = emmus::memory::identifiers::FrameId;

    /**
     * @brief Creates an empty page table.
     */
    PageTable() = default;

    /**
     * @brief Creates a mapping between a virtual page and physical frame.
     *
     * A page that is already mapped is not remapped by this operation.
     *
     * @param pageId Virtual page identifier.
     * @param frameId Physical frame identifier.
     *
     * @return true if the mapping was added; false if the page was already
     *         mapped.
     */
    [[nodiscard]]
    bool map(PageId pageId, FrameId frameId);

    /**
     * @brief Updates an existing page-to-frame mapping.
     *
     * This operation does not create a new mapping. If the page is not
     * currently mapped, the operation fails.
     *
     * @param pageId Virtual page identifier.
     * @param frameId New physical frame identifier.
     *
     * @return true if the existing mapping was updated; false if the page
     *         was not mapped.
     */
    [[nodiscard]]
    bool updateMapping(PageId pageId, FrameId frameId);

    /**
     * @brief Retrieves the physical frame mapped to a virtual page.
     *
     * @param pageId Virtual page identifier.
     *
     * @return The mapped FrameId, or std::nullopt if the page is unmapped.
     */
    [[nodiscard]]
    std::optional<FrameId> lookup(PageId pageId) const noexcept;

    /**
     * @brief Determines whether a virtual page currently has a mapping.
     *
     * @param pageId Virtual page identifier.
     *
     * @return true if the page is mapped; otherwise false.
     */
    [[nodiscard]]
    bool isMapped(PageId pageId) const noexcept;

    /**
     * @brief Removes the mapping for a virtual page.
     *
     * @param pageId Virtual page identifier.
     *
     * @return true if a mapping was removed; false if the page was already
     *         unmapped.
     */
    [[nodiscard]]
    bool unmap(PageId pageId) noexcept;

    /**
     * @brief Returns the number of active page-to-frame mappings.
     *
     * @return Number of mappings currently maintained by the page table.
     */
    [[nodiscard]]
    std::size_t size() const noexcept;

    /**
     * @brief Determines whether the page table contains no mappings.
     *
     * @return true if no mappings exist; otherwise false.
     */
    [[nodiscard]]
    bool empty() const noexcept;

private:
    std::unordered_map<PageId, FrameId> mappings_;
};

} // namespace emmus::memory::mmu