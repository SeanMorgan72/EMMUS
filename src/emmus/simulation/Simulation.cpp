#include "emmus/simulation/Simulation.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "emmus/algorithms/replacement/ClockPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/LRUPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/OptimalPageReplacementPolicy.hpp"
#include "emmus/infrastructure/configuration/SimulationConfigurationValidator.hpp"
#include "emmus/simulation/workload/LocalityWorkload.hpp"
#include "emmus/simulation/workload/MixedWorkload.hpp"
#include "emmus/simulation/workload/MultiProcessWorkload.hpp"
#include "emmus/simulation/workload/RandomWorkload.hpp"
#include "emmus/simulation/workload/SequentialWorkload.hpp"

namespace emmus::simulation {

namespace {

using PolicyType =
    algorithms::replacement::PageReplacementPolicyType;

} // namespace

Simulation::Simulation(
    infrastructure::configuration::SimulationConfiguration configuration)
    : configuration_(std::move(configuration)),
      pageTable_(),
      physicalMemoryManager_(configuration_.frameCount().value()) {

    const auto errors =
        infrastructure::configuration::
            SimulationConfigurationValidator::validate(configuration_);

    if (!errors.empty()) {
        throw std::invalid_argument(errors.front());
    }
}

void Simulation::initialize() {
    if (initialized_) {
        return;
    }

    processManager_.clear();

    replacementPolicyFactory_.clear();

    if (!replacementPolicyFactory_.registerPolicy(
            PolicyType::FIFO,
            [] {
                return std::make_unique<
                    algorithms::replacement::
                        FIFOPageReplacementPolicy>();
            })) {

        throw std::runtime_error(
            "Failed to register FIFO page replacement policy.");
    }

    if (!replacementPolicyFactory_.registerPolicy(
            PolicyType::LRU,
            [] {
                return std::make_unique<
                    algorithms::replacement::
                        LRUPageReplacementPolicy>();
            })) {

        throw std::runtime_error(
            "Failed to register LRU page replacement policy.");
    }

    if (!replacementPolicyFactory_.registerPolicy(
            PolicyType::CLOCK,
            [] {
                return std::make_unique<
                    algorithms::replacement::
                        ClockPageReplacementPolicy>();
            })) {

        throw std::runtime_error(
            "Failed to register Clock page replacement policy.");
    }

    if (!replacementPolicyFactory_.registerPolicy(
            PolicyType::OPTIMAL,
            [] {
                return std::make_unique<
                    algorithms::replacement::
                        OptimalPageReplacementPolicy>();
            })) {

        throw std::runtime_error(
            "Failed to register Optimal page replacement policy.");
    }

    replacementPolicy_ = createReplacementPolicy();

    if (!replacementPolicy_) {
        throw std::runtime_error(
            "Unable to create configured page replacement policy.");
    }

    mmu_ = std::make_unique<
        memory::mmu::MemoryManagementUnit>(
        pageTable_,
        physicalMemoryManager_,
        *replacementPolicy_,
        configuration_.pageSize());

    registerPages();

    workload_ = createWorkload();

    if (!workload_) {
        throw std::runtime_error(
            "Unable to create configured simulation workload.");
    }

    accessExecutor_ =
        std::make_unique<application::MemoryAccessExecutor>(*mmu_);

    initialized_ = true;
}

void Simulation::registerPages() {
    std::uint64_t nextPageId = 1;

    for (std::size_t processIndex = 0;
        processIndex < configuration_.processCount();
        ++processIndex)
    {
        auto& process =
            processManager_.createProcess(
                configuration_.pageCountPerProcess(),
                configuration_.pageSize().value());

        for (std::size_t pageIndex = 0;
            pageIndex < configuration_.pageCountPerProcess();
            ++pageIndex)
        {
            const auto pageId =
                memory::identifiers::PageId{nextPageId++};

            memory::virtual_memory::Page page(
                pageId,
                process.processId());

            mmu_->registerPage(
                page,
                static_cast<std::uint64_t>(pageIndex));
        }
    }
}

std::unique_ptr<
    algorithms::replacement::IPageReplacementPolicy>
Simulation::createReplacementPolicy() {

    return replacementPolicyFactory_.create(
        configuration_.replacementPolicy());
}

std::unique_ptr<workload::IWorkload>
Simulation::createWorkload() {

    std::vector<
        std::unique_ptr<workload::IWorkload>>
        workloads;

    workloads.reserve(configuration_.processCount());

    const std::size_t processCount =
        configuration_.processCount();

    const std::size_t totalAccessCount =
        configuration_.memoryAccessCount();

    /*
     * Distribute the configured total number of accesses
     * across all processes.
     *
     * Example:
     *
     *   10 accesses / 3 processes
     *
     *   Process 1 -> 4
     *   Process 2 -> 3
     *   Process 3 -> 3
     *
     * This guarantees that the total number of generated
     * accesses remains exactly equal to memoryAccessCount().
     */
    const std::size_t baseAccessCount =
        totalAccessCount / processCount;

    const std::size_t remainder =
        totalAccessCount % processCount;

    for (std::size_t processIndex = 0;
         processIndex < processCount;
         ++processIndex) {

        const auto processId =
            memory::identifiers::ProcessId{
                static_cast<std::uint64_t>(
                    processIndex + 1)};

        const std::size_t processAccessCount =
            baseAccessCount +
            (processIndex < remainder ? 1U : 0U);

        switch (configuration_.workloadType()) {

        case infrastructure::configuration::
            WorkloadType::Sequential:

            workloads.push_back(
                std::make_unique<
                    workload::SequentialWorkload>(
                    processId,
                    configuration_.pageCountPerProcess(),
                    configuration_.pageSize(),
                    processAccessCount));

            break;

        case infrastructure::configuration::
            WorkloadType::Random:

            workloads.push_back(
                std::make_unique<
                    workload::RandomWorkload>(
                    processId,
                    configuration_.pageCountPerProcess(),
                    configuration_.pageSize(),
                    processAccessCount,
                    configuration_.randomSeed() +
                        static_cast<std::uint64_t>(
                            processIndex)));

            break;

        case infrastructure::configuration::
            WorkloadType::Locality:

            workloads.push_back(
                std::make_unique<
                    workload::LocalityWorkload>(
                    processId,
                    configuration_.pageCountPerProcess(),
                    configuration_.pageSize(),
                    processAccessCount,
                    configuration_.temporalLocalityStrength(),
                    configuration_.spatialLocalityStrength(),
                    configuration_.workingSetSize(),
                    configuration_.randomSeed() +
                        static_cast<std::uint64_t>(
                            processIndex)));

            break;

        case infrastructure::configuration::
            WorkloadType::Mixed: {

            const auto& mixedSegments =
                configuration_.mixedWorkloadSegments();

            std::vector<
                std::unique_ptr<workload::IWorkload>>
                segments;

            segments.reserve(mixedSegments.size());

            std::size_t allocatedAccesses = 0;

            for (std::size_t segmentIndex = 0;
                 segmentIndex < mixedSegments.size();
                 ++segmentIndex) {

                const auto& segment =
                    mixedSegments[segmentIndex];

                const std::size_t remainingAccesses =
                    processAccessCount -
                    allocatedAccesses;

                if (remainingAccesses == 0) {
                    break;
                }

                const std::size_t segmentAccessCount =
                    std::min(
                        segment.accessCount,
                        remainingAccesses);

                if (segmentAccessCount == 0) {
                    continue;
                }

                const std::uint64_t seed =
                    configuration_.randomSeed() +
                    static_cast<std::uint64_t>(
                        processIndex * 1000003ULL +
                        segmentIndex);

                switch (segment.workloadType) {

                case infrastructure::configuration::
                    WorkloadType::Sequential:

                    segments.push_back(
                        std::make_unique<
                            workload::SequentialWorkload>(
                            processId,
                            configuration_.pageCountPerProcess(),
                            configuration_.pageSize(),
                            segmentAccessCount));

                    break;

                case infrastructure::configuration::
                    WorkloadType::Random:

                    segments.push_back(
                        std::make_unique<
                            workload::RandomWorkload>(
                            processId,
                            configuration_.pageCountPerProcess(),
                            configuration_.pageSize(),
                            segmentAccessCount,
                            seed));

                    break;

                case infrastructure::configuration::
                    WorkloadType::Locality:

                    segments.push_back(
                        std::make_unique<
                            workload::LocalityWorkload>(
                            processId,
                            configuration_.pageCountPerProcess(),
                            configuration_.pageSize(),
                            segmentAccessCount,
                            configuration_.
                                temporalLocalityStrength(),
                            configuration_.
                                spatialLocalityStrength(),
                            configuration_.workingSetSize(),
                            seed));

                    break;

                case infrastructure::configuration::
                    WorkloadType::Mixed:

                    throw std::logic_error(
                        "Nested mixed workloads are not supported.");
                }

                allocatedAccesses +=
                    segmentAccessCount;
            }

            workloads.push_back(
                std::make_unique<
                    workload::MixedWorkload>(
                    std::move(segments)));

            break;
        }

        default:

            throw std::invalid_argument(
                "Unsupported simulation workload type.");
        }
    }

    return std::make_unique<
        workload::MultiProcessWorkload>(
        std::move(workloads));
}

SimulationResult Simulation::run() {
    const auto start =
        std::chrono::steady_clock::now();

    /*
     * Each call to run() represents a fresh simulation using
     * the same configuration.
     *
     * Destroy objects that reference the state being reset before
     * clearing/rebuilding that state.
     */
    accessExecutor_.reset();
    workload_.reset();
    mmu_.reset();
    replacementPolicy_.reset();

    pageTable_.clear();
    physicalMemoryManager_.clear();

    initialized_ = false;

    initialize();

    std::vector<memory::access::MemoryAccess> accesses;

    accesses.reserve(workload_->size());

    while (workload_->hasNext()) {
        accesses.push_back(
            workload_->nextAccess());
    }

    const auto executionResult =
        accessExecutor_->execute(accesses);

    const auto end =
        std::chrono::steady_clock::now();

    const emmus::statistics::PageFaultStatistics pageFaultStatistics =
        mmu_->pageFaultStatistics();

    const emmus::statistics::PageReplacementStatistics pageReplacementStatistics =
        replacementPolicy_->statistics();

    return SimulationResult(
        configuration_,
        executionResult,
        pageFaultStatistics,
        pageReplacementStatistics,
        std::chrono::duration_cast<
            std::chrono::nanoseconds>(
            end - start),
        SimulationStatus::Completed);
}

} // namespace emmus::simulation
