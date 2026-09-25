#include "SimulationActivityVisualizationView.hpp"

#include <sstream>
#include <string>

namespace emmus::gui
{

void SimulationActivityVisualizationView::update(const ActivityLog& log)
{
    log_ = log;
}

const SimulationActivityVisualizationView::ActivityLog&
SimulationActivityVisualizationView::log() const noexcept
{
    return log_;
}

std::string SimulationActivityVisualizationView::render() const
{
    std::ostringstream stream;
    stream << "Simulation Activity\n";

    if (log_.empty())
    {
        stream << "[no activity recorded]\n";
        return stream.str();
    }

    for (const auto& event : log_.events())
    {
        std::string level = "INFO";
        switch (event.type)
        {
            case ActivityType::Access:
                level = "ACCESS";
                break;
            case ActivityType::PageFault:
                level = "FAULT";
                break;
            case ActivityType::PageReplacement:
                level = "REPLACE";
                break;
            case ActivityType::FrameAssignment:
                level = "FRAME";
                break;
            case ActivityType::DirtyEviction:
                level = "DIRTY";
                break;
            case ActivityType::Status:
            default:
                level = "STATUS";
                break;
        }

        stream << "[" << level << "] " << event.render() << "\n";
    }

    return stream.str();
}

std::string SimulationActivityVisualizationView::renderCompact() const
{
    std::ostringstream stream;
    stream << "{events=" << log_.size() << "}";
    if (!log_.empty())
    {
        const auto& last = log_.events().back();
        stream << " last=" << last.summary;
    }
    return stream.str();
}

} // namespace emmus::gui
