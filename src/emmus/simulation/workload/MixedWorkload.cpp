#include "emmus/simulation/workload/MixedWorkload.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace emmus::simulation::workload {

MixedWorkload::MixedWorkload(
    std::vector<std::unique_ptr<IWorkload>> segments)
    : segments_(std::move(segments)) {
    for (const auto& segment : segments_) {
        if (segment == nullptr) {
            throw std::invalid_argument(
                "Mixed workload cannot contain a null segment.");
        }

        const std::size_t segmentSize = segment->size();

        if (totalSize_ >
            std::numeric_limits<std::size_t>::max() - segmentSize) {
            throw std::overflow_error(
                "Mixed workload size exceeds representable range.");
        }

        totalSize_ += segmentSize;
    }
}

bool MixedWorkload::hasNext() const noexcept {
    std::size_t segmentIndex = currentSegment_;

    while (segmentIndex < segments_.size()) {
        if (segments_[segmentIndex]->hasNext()) {
            return true;
        }

        ++segmentIndex;
    }

    return false;
}

memory::access::MemoryAccess MixedWorkload::nextAccess() {
    while (currentSegment_ < segments_.size()) {
        if (segments_[currentSegment_]->hasNext()) {
            return segments_[currentSegment_]->nextAccess();
        }

        ++currentSegment_;
    }

    throw std::out_of_range(
        "Mixed workload has no remaining accesses.");
}

void MixedWorkload::reset() {
    for (auto& segment : segments_) {
        segment->reset();
    }

    currentSegment_ = 0;
}

std::size_t MixedWorkload::size() const noexcept {
    return totalSize_;
}

} // namespace emmus::simulation::workload