#pragma once

#include <optional>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"

namespace emmus::memory::virtual_memory
{

/**
 * @brief Error conditions for invalid Page state transitions.
 */
enum class PageError
{
    AlreadyResident,
    AlreadyNonResident
};


/**
 * @brief Represents one virtual-memory page.
 *
 * Page models the identity and virtual-memory state of a page belonging to a
 * simulated process. Physical-memory management, page-table management, and
 * page-replacement decisions belong to their respective components.
 */
class Page
{
public:

    using PageId = identifiers::PageId;
    using ProcessId = identifiers::ProcessId;
    using FrameId = identifiers::FrameId;


    /**
     * @brief Constructs a non-resident, clean, unreferenced page.
     *
     * @param pageId Unique page identifier within the owning process context.
     * @param processId Identifier of the process that owns the page.
     */
    Page(
        PageId pageId,
        ProcessId processId
    ) noexcept;


    Page(const Page&) = default;

    Page(Page&&) noexcept = default;

    Page& operator=(const Page&) = default;

    Page& operator=(Page&&) noexcept = default;

    ~Page() = default;


    /**
     * @brief Returns the page identity.
     */
    [[nodiscard]]
    PageId id() const noexcept;


    /**
     * @brief Returns the owning process identity.
     */
    [[nodiscard]]
    ProcessId processId() const noexcept;


    /**
     * @brief Returns whether the page is currently resident.
     */
    [[nodiscard]]
    bool isResident() const noexcept;


    /**
     * @brief Returns the frame associated with the page.
     *
     * @return The mapped FrameId when resident, otherwise std::nullopt.
     */
    [[nodiscard]]
    const std::optional<FrameId>& mappedFrame() const noexcept;


    /**
     * @brief Returns whether the page is dirty.
     */
    [[nodiscard]]
    bool isDirty() const noexcept;


    /**
     * @brief Returns whether the page has been referenced/accessed.
     */
    [[nodiscard]]
    bool isReferenced() const noexcept;


    /**
     * @brief Associates the page with a physical frame.
     *
     * A page must be non-resident before it can be mapped.
     *
     * @return std::nullopt on success.
     *
     * @return PageError::AlreadyResident when already resident.
     */
    [[nodiscard]]
    std::optional<PageError> mapToFrame(
        FrameId frameId
    ) noexcept;


    /**
     * @brief Removes the page's physical-frame association.
     *
     * Unmapping returns the page to its non-resident initial state: no frame,
     * clean, and unreferenced.
     *
     * @return std::nullopt on success.
     *
     * @return PageError::AlreadyNonResident when already non-resident.
     */
    [[nodiscard]]
    std::optional<PageError> unmapFromFrame() noexcept;


    /**
     * @brief Marks the page as dirty.
     */
    void markDirty() noexcept;


    /**
     * @brief Clears the dirty state.
     */
    void clearDirty() noexcept;


    /**
     * @brief Marks the page as referenced/accessed.
     */
    void markReferenced() noexcept;


    /**
     * @brief Clears the reference/access state.
     */
    void clearReferenced() noexcept;


private:

    PageId pageId_;

    ProcessId processId_;

    std::optional<FrameId> mappedFrame_;

    bool dirty_;

    bool referenced_;
};

} // namespace emmus::memory::virtual_memory