#include "PhysicalMemoryVisualizationView.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace emmus::gui
{

void PhysicalMemoryVisualizationView::update(const Manager& manager)
{
    update(manager.snapshotUtilization());
}

void PhysicalMemoryVisualizationView::update(const Utilization& snapshot)
{
    snapshot_ = snapshot;
}

const PhysicalMemoryVisualizationView::Utilization&
PhysicalMemoryVisualizationView::snapshot() const noexcept
{
    return snapshot_;
}

std::string PhysicalMemoryVisualizationView::render() const
{
    std::ostringstream stream;
    stream << "Physical Memory Utilization\n";
    stream << "Total Frames: " << snapshot_.totalFrames << "\n";
    stream << "Allocated: " << snapshot_.allocatedFrames << "\n";
    stream << "Free: " << snapshot_.freeFrames << "\n";
    stream << "Utilization: " << std::fixed << std::setprecision(1)
           << snapshot_.utilizationPercent << "%\n";

    if (snapshot_.frames.empty())
    {
        stream << "[no frames]";
        return stream.str();
    }

    for (const auto& frame : snapshot_.frames)
    {
        const auto label = frame.pageId.has_value()
            ? std::to_string(frame.pageId->value())
            : "-";

        stream << "F" << frame.frameId.value() << " ["
               << (frame.isAllocated ? "ALLOC" : "FREE") << "] "
               << "page=" << label << "\n";
    }

    return stream.str();
}

std::string PhysicalMemoryVisualizationView::renderCompact() const
{
    std::ostringstream stream;

    stream << "[";
    for (std::size_t index = 0; index < snapshot_.frames.size(); ++index)
    {
        const auto& frame = snapshot_.frames[index];
        stream << (frame.isAllocated ? "1" : "0");
        if (index + 1U < snapshot_.frames.size())
        {
            stream << " ";
        }
    }
    stream << "]";

    return stream.str();
}

} // namespace emmus::gui
