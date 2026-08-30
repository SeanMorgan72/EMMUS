#pragma once

#include <cstdint>
#include <functional>

namespace emmus::memory::identifiers
{

/**
 * @brief Strongly typed identifier for a simulated process.
 */
class ProcessId
{
public:
    using ValueType = std::uint64_t;

    constexpr explicit ProcessId(ValueType value) noexcept
        : value_(value)
    {
    }

    [[nodiscard]]
    constexpr ValueType value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr bool operator==(const ProcessId& other) const noexcept
    {
        return value_ == other.value_;
    }

private:
    ValueType value_;
};


/**
 * @brief Strongly typed identifier for a virtual-memory page.
 */
class PageId
{
public:
    using ValueType = std::uint64_t;

    constexpr explicit PageId(ValueType value) noexcept
        : value_(value)
    {
    }

    [[nodiscard]]
    constexpr ValueType value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr bool operator==(const PageId& other) const noexcept
    {
        return value_ == other.value_;
    }

private:
    ValueType value_;
};


/**
 * @brief Strongly typed identifier for a physical-memory frame.
 */
class FrameId
{
public:
    using ValueType = std::uint64_t;

    constexpr explicit FrameId(ValueType value) noexcept
        : value_(value)
    {
    }

    [[nodiscard]]
    constexpr ValueType value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr bool operator==(const FrameId& other) const noexcept
    {
        return value_ == other.value_;
    }

private:
    ValueType value_;
};

} // namespace emmus::memory::identifiers


namespace std
{

template <>
struct hash<emmus::memory::identifiers::ProcessId>
{
    [[nodiscard]]
    std::size_t operator()(
        const emmus::memory::identifiers::ProcessId& id
    ) const noexcept
    {
        return std::hash<
            emmus::memory::identifiers::ProcessId::ValueType
        >{}(id.value());
    }
};


template <>
struct hash<emmus::memory::identifiers::PageId>
{
    [[nodiscard]]
    std::size_t operator()(
        const emmus::memory::identifiers::PageId& id
    ) const noexcept
    {
        return std::hash<
            emmus::memory::identifiers::PageId::ValueType
        >{}(id.value());
    }
};


template <>
struct hash<emmus::memory::identifiers::FrameId>
{
    [[nodiscard]]
    std::size_t operator()(
        const emmus::memory::identifiers::FrameId& id
    ) const noexcept
    {
        return std::hash<
            emmus::memory::identifiers::FrameId::ValueType
        >{}(id.value());
    }
};

} // namespace std