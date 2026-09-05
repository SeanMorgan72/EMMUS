#pragma once

#include <cstddef>
#include <memory>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"

namespace emmus::tests {

/**
 * @brief Common fixture for Memory Management Unit tests.
 *
 * The fixture owns the lower-level memory-management components used by the
 * MMU. The concrete MMU is intentionally not included yet; this fixture
 * establishes the deterministic test environment that its tests will use.
 */
class MemoryManagementUnitFixture : public ::testing::Test {
protected:
    using PageTable = emmus::memory::mmu::PageTable;
    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;
    using FIFOPageReplacementPolicy =
        emmus::algorithms::replacement::FIFOPageReplacementPolicy;

    using ProcessId = emmus::memory::identifiers::ProcessId;
    using PageId = emmus::memory::identifiers::PageId;
    using FrameId = emmus::memory::identifiers::FrameId;

    static constexpr ProcessId kProcess1{100};
    static constexpr ProcessId kProcess2{200};

    static constexpr PageId kPage1{1};
    static constexpr PageId kPage2{2};
    static constexpr PageId kPage3{3};
    static constexpr PageId kPage4{4};

    static constexpr FrameId kFrame1{0};
    static constexpr FrameId kFrame2{1};
    static constexpr FrameId kFrame3{2};

    // Small deterministic capacity makes replacement scenarios easy to test.
    static constexpr std::size_t kFrameCount = 2;

    void SetUp() override
    {
        pageTable_ = std::make_unique<PageTable>();
        physicalMemoryManager_ =
            std::make_unique<PhysicalMemoryManager>(kFrameCount);
        replacementPolicy_ =
            std::make_unique<FIFOPageReplacementPolicy>();
    }

    void TearDown() override
    {
        replacementPolicy_.reset();
        physicalMemoryManager_.reset();
        pageTable_.reset();
    }

    [[nodiscard]] PageTable& pageTable() noexcept
    {
        return *pageTable_;
    }

    [[nodiscard]] const PageTable& pageTable() const noexcept
    {
        return *pageTable_;
    }

    [[nodiscard]] PhysicalMemoryManager& physicalMemoryManager() noexcept
    {
        return *physicalMemoryManager_;
    }

    [[nodiscard]] const PhysicalMemoryManager&
    physicalMemoryManager() const noexcept
    {
        return *physicalMemoryManager_;
    }

    [[nodiscard]] FIFOPageReplacementPolicy&
    replacementPolicy() noexcept
    {
        return *replacementPolicy_;
    }

    [[nodiscard]] const FIFOPageReplacementPolicy&
    replacementPolicy() const noexcept
    {
        return *replacementPolicy_;
    }

private:
    std::unique_ptr<PageTable> pageTable_;
    std::unique_ptr<PhysicalMemoryManager> physicalMemoryManager_;
    std::unique_ptr<FIFOPageReplacementPolicy> replacementPolicy_;
};

} // namespace emmus::tests
