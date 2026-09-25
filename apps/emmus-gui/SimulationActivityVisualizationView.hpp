#pragma once

#include <string>
#include <vector>

#include "emmus/simulation/activity/SimulationActivityLog.hpp"

namespace emmus::gui
{

class SimulationActivityVisualizationView
{
public:
    using ActivityLog = emmus::simulation::activity::SimulationActivityLog;
    using ActivityEvent = emmus::simulation::activity::SimulationActivityEvent;
    using ActivityType = emmus::simulation::activity::SimulationActivityType;

    SimulationActivityVisualizationView() = default;

    void update(const ActivityLog& log);

    [[nodiscard]] const ActivityLog& log() const noexcept;

    [[nodiscard]] std::string render() const;

    [[nodiscard]] std::string renderCompact() const;

private:
    ActivityLog log_{};
};

} // namespace emmus::gui
