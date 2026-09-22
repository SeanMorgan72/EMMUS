#pragma once

#include <memory>
#include <string>
#include <vector>

#include "emmus/application/MemoryAccessExecutor.hpp"
#include "emmus/algorithms/replacement/PageReplacementPolicyFactory.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/memory/mmu/MemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/simulation/ProcessManager.hpp"
#include "emmus/simulation/SimulationResult.hpp"
#include "emmus/simulation/workload/IWorkload.hpp"

namespace emmus::simulation {

class Simulation final {
public:
    explicit Simulation(
        infrastructure::configuration::SimulationConfiguration configuration);

    [[nodiscard]] SimulationResult run();

    [[nodiscard]] memory::physical::PhysicalMemoryManager&
    physicalMemoryManager() noexcept;

    [[nodiscard]] const memory::physical::PhysicalMemoryManager&
    physicalMemoryManager() const noexcept;

private:
    void initialize();

    void registerPages();

    [[nodiscard]] std::unique_ptr<
        algorithms::replacement::IPageReplacementPolicy>
    createReplacementPolicy();

    [[nodiscard]] std::unique_ptr<workload::IWorkload>
    createWorkload();

    infrastructure::configuration::SimulationConfiguration configuration_;

    ProcessManager processManager_;

    memory::mmu::PageTable pageTable_;
    memory::physical::PhysicalMemoryManager physicalMemoryManager_;

    algorithms::replacement::PageReplacementPolicyFactory
        replacementPolicyFactory_;

    std::unique_ptr<
        algorithms::replacement::IPageReplacementPolicy>
        replacementPolicy_;

    std::unique_ptr<memory::mmu::MemoryManagementUnit> mmu_;

    std::unique_ptr<workload::IWorkload> workload_;

    std::unique_ptr<application::MemoryAccessExecutor>
        accessExecutor_;

    bool initialized_{false};
};

} // namespace emmus::simulation