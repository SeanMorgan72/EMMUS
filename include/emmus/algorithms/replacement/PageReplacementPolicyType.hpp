#pragma once

#include <cstdint>

namespace emmus::algorithms::replacement
{

/**
 * @brief Identifies a page-replacement policy implementation.
 *
 * PageReplacementPolicyType is used by the page-replacement factory
 * to select a concrete policy without exposing concrete policy types
 * to callers.
 *
 * Concrete policy implementations are introduced by their respective
 * implementation tasks and registered with PageReplacementPolicyFactory.
 */
enum class PageReplacementPolicyType : std::uint8_t
{
    FIFO = 0,
    LRU,
    CLOCK,
    OPTIMAL
};

} // namespace emmus::algorithms::replacement