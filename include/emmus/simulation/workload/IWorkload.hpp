#pragma once

#include <cstddef>

#include "emmus/memory/access/MemoryAccess.hpp"

namespace emmus::simulation::workload {

class IWorkload {
public:
    virtual ~IWorkload() = default;

    [[nodiscard]] virtual bool hasNext() const noexcept = 0;

    virtual memory::access::MemoryAccess nextAccess() = 0;

    virtual void reset() = 0;

    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
};

} // namespace emmus::simulation::workload