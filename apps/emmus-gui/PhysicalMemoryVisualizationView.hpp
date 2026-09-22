#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/physical/PhysicalMemoryUtilization.hpp"

namespace emmus::gui
{

class PhysicalMemoryVisualizationView
{
public:
    using Manager = emmus::memory::physical::PhysicalMemoryManager;
    using Utilization = emmus::memory::physical::PhysicalMemoryUtilization;

    PhysicalMemoryVisualizationView() = default;

    void update(const Manager& manager);

    void update(const Utilization& snapshot);

    [[nodiscard]]
    const Utilization& snapshot() const noexcept;

    [[nodiscard]]
    std::string render() const;

    [[nodiscard]]
    std::string renderCompact() const;

private:
    Utilization snapshot_{};
};

} // namespace emmus::gui
