#include "emmus/simulation/workload/MultiProcessWorkload.hpp"

#include <stdexcept>
#include <utility>

namespace emmus::simulation::workload {

MultiProcessWorkload::MultiProcessWorkload(
    std::vector<std::unique_ptr<IWorkload>> workloads)
    : workloads_(std::move(workloads)) {

    for (const auto& workload : workloads_) {
        if (workload == nullptr) {
            throw std::invalid_argument(
                "Multi-process workload cannot contain a null workload.");
        }

        totalSize_ += workload->size();
    }
}

bool MultiProcessWorkload::hasNext() const noexcept {
    while (currentWorkload_ < workloads_.size() &&
           !workloads_[currentWorkload_]->hasNext()) {
        ++currentWorkload_;
    }

    return currentWorkload_ < workloads_.size();
}

memory::access::MemoryAccess MultiProcessWorkload::nextAccess() {
    if (!hasNext()) {
        throw std::out_of_range(
            "Multi-process workload has no remaining accesses.");
    }

    return workloads_[currentWorkload_]->nextAccess();
}

void MultiProcessWorkload::reset() {
    for (auto& workload : workloads_) {
        workload->reset();
    }

    currentWorkload_ = 0;
}

std::size_t MultiProcessWorkload::size() const noexcept {
    return totalSize_;
}

} // namespace emmus::simulation::workload