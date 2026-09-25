#include "emmus/simulation/activity/SimulationActivityLog.hpp"

#include <sstream>
#include <string>

namespace emmus::simulation::activity
{
namespace
{
std::string operationToString(
    const SimulationActivityEvent::MemoryAccessOperation operation)
{
    switch (operation)
    {
        case emmus::memory::access::MemoryAccessOperation::Read:
            return "READ";
        case emmus::memory::access::MemoryAccessOperation::Write:
            return "WRITE";
    }

    return "UNKNOWN";
}

std::string activityTypeToString(SimulationActivityType type)
{
    switch (type)
    {
        case SimulationActivityType::Access:
            return "ACCESS";
        case SimulationActivityType::PageFault:
            return "PAGE_FAULT";
        case SimulationActivityType::PageReplacement:
            return "REPLACEMENT";
        case SimulationActivityType::FrameAssignment:
            return "FRAME_ASSIGNMENT";
        case SimulationActivityType::DirtyEviction:
            return "DIRTY_EVICTION";
        case SimulationActivityType::Status:
            return "STATUS";
    }

    return "UNKNOWN";
}
} // namespace

std::string SimulationActivityEvent::render() const
{
    std::ostringstream stream;
    stream << "#" << sequence << " " << activityTypeToString(type);

    if (processId.has_value())
    {
        stream << " P" << processId->value();
    }

    if (pageId.has_value())
    {
        stream << " page=" << pageId->value();
    }

    if (frameId.has_value())
    {
        stream << " frame=" << frameId->value();
    }

    if (operation.has_value())
    {
        stream << " op=" << operationToString(operation.value());
    }

    if (!summary.empty())
    {
        stream << " - " << summary;
    }

    if (!detail.empty())
    {
        stream << " :: " << detail;
    }

    return stream.str();
}

void SimulationActivityLog::record(const SimulationActivityEvent& event)
{
    SimulationActivityEvent stored = event;
    stored.sequence = nextSequence_++;
    events_.push_back(stored);
}

void SimulationActivityLog::recordAccess(
    emmus::memory::identifiers::ProcessId processId,
    emmus::memory::identifiers::PageId pageId,
    std::optional<emmus::memory::identifiers::FrameId> frameId,
    emmus::memory::access::MemoryAccessOperation operation,
    bool success,
    bool pageFault,
    bool pageReplacement,
    bool dirtyEviction,
    std::string detail)
{
    SimulationActivityEvent event{};
    event.type = SimulationActivityType::Access;
    event.processId = processId;
    event.pageId = pageId;
    event.frameId = frameId;
    event.operation = operation;
    event.success = success;
    event.pageFault = pageFault;
    event.pageReplacement = pageReplacement;
    event.dirtyEviction = dirtyEviction;
    event.summary = success ? "Access processed" : "Access failed";
    event.detail = std::move(detail);
    record(event);
}

void SimulationActivityLog::recordPageFault(
    emmus::memory::identifiers::ProcessId processId,
    emmus::memory::identifiers::PageId pageId,
    std::optional<emmus::memory::identifiers::FrameId> frameId,
    std::string detail)
{
    SimulationActivityEvent event{};
    event.type = SimulationActivityType::PageFault;
    event.processId = processId;
    event.pageId = pageId;
    event.frameId = frameId;
    event.summary = "Page fault";
    event.detail = std::move(detail);
    record(event);
}

void SimulationActivityLog::recordPageReplacement(
    emmus::memory::identifiers::ProcessId processId,
    emmus::memory::identifiers::PageId pageId,
    emmus::memory::identifiers::FrameId victimFrameId,
    emmus::memory::identifiers::PageId victimPageId,
    std::string detail)
{
    SimulationActivityEvent event{};
    event.type = SimulationActivityType::PageReplacement;
    event.processId = processId;
    event.pageId = pageId;
    event.victimPageId = victimPageId;
    event.frameId = victimFrameId;
    event.victimFrameId = victimFrameId;
    event.summary = "Page replacement";
    event.detail = std::move(detail);
    event.pageReplacement = true;
    record(event);
}

void SimulationActivityLog::recordFrameAssignment(
    emmus::memory::identifiers::ProcessId processId,
    emmus::memory::identifiers::PageId pageId,
    emmus::memory::identifiers::FrameId frameId,
    std::string detail)
{
    SimulationActivityEvent event{};
    event.type = SimulationActivityType::FrameAssignment;
    event.processId = processId;
    event.pageId = pageId;
    event.frameId = frameId;
    event.summary = "Frame assigned";
    event.detail = std::move(detail);
    record(event);
}

void SimulationActivityLog::recordDirtyEviction(
    emmus::memory::identifiers::ProcessId processId,
    emmus::memory::identifiers::PageId pageId,
    emmus::memory::identifiers::FrameId frameId,
    std::string detail)
{
    SimulationActivityEvent event{};
    event.type = SimulationActivityType::DirtyEviction;
    event.processId = processId;
    event.pageId = pageId;
    event.frameId = frameId;
    event.summary = "Dirty eviction";
    event.detail = std::move(detail);
    event.dirtyEviction = true;
    record(event);
}

void SimulationActivityLog::recordStatus(
    emmus::memory::identifiers::ProcessId processId,
    std::string detail,
    std::optional<emmus::memory::identifiers::PageId> pageId,
    std::optional<emmus::memory::identifiers::FrameId> frameId,
    bool success)
{
    SimulationActivityEvent event{};
    event.type = SimulationActivityType::Status;
    event.processId = processId;
    event.pageId = pageId;
    event.frameId = frameId;
    event.success = success;
    event.summary = success ? "Status" : "Status warning";
    event.detail = std::move(detail);
    record(event);
}

void SimulationActivityLog::clear() noexcept
{
    events_.clear();
    nextSequence_ = 0U;
}

std::size_t SimulationActivityLog::size() const noexcept
{
    return events_.size();
}

bool SimulationActivityLog::empty() const noexcept
{
    return events_.empty();
}

const std::vector<SimulationActivityEvent>&
SimulationActivityLog::events() const noexcept
{
    return events_;
}

std::string SimulationActivityLog::render() const
{
    std::ostringstream stream;
    for (const auto& event : events_)
    {
        stream << event.render() << '\n';
    }
    return stream.str();
}

std::string SimulationActivityLog::renderCompact() const
{
    std::ostringstream stream;
    for (const auto& event : events_)
    {
        stream << event.render() << "; ";
    }
    return stream.str();
}

} // namespace emmus::simulation::activity
