#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "emmus/algorithms/replacement/ClockPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/LRUPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/OptimalPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/PageReplacementPolicyFactory.hpp"
#include "emmus/infrastructure/configuration/BenchmarkConfiguration.hpp"
#include "emmus/memory/access/MemoryAccess.hpp"
#include "emmus/memory/mmu/MemoryManagementUnit.hpp"
#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "emmus/memory/virtual_memory/Page.hpp"
#include "emmus/simulation/Simulation.hpp"
#include "emmus/simulation/SimulationComparison.hpp"
#include "emmus/simulation/activity/SimulationActivityLog.hpp"

namespace
{

using emmus::algorithms::replacement::PageReplacementPolicyFactory;
using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::infrastructure::configuration::BenchmarkConfiguration;
using emmus::infrastructure::configuration::WorkloadType;
using emmus::memory::access::FrameCount;
using emmus::memory::access::MemoryAccess;
using emmus::memory::access::MemoryAccessOperation;
using emmus::memory::access::PageSize;
using emmus::memory::access::VirtualAddress;
using emmus::memory::identifiers::PageId;
using emmus::memory::identifiers::ProcessId;
using emmus::memory::mmu::MemoryManagementUnit;
using emmus::memory::mmu::PageTable;
using emmus::memory::physical::PhysicalMemoryManager;
using emmus::memory::virtual_memory::Page;
using emmus::simulation::Simulation;
using emmus::simulation::SimulationComparison;
using emmus::simulation::activity::SimulationActivityLog;

constexpr PageSize kPageSize{4096U};
constexpr ProcessId kProcessId{1U};
constexpr std::array<std::uint64_t, 12U> kReferenceTrace{
    1U, 2U, 3U, 4U, 1U, 2U, 5U, 1U, 2U, 3U, 4U, 5U};

struct TraceResult
{
    std::size_t faults;
    std::size_t replacements;
    std::size_t dirtyEvictions;
    std::chrono::nanoseconds policyTime;
    emmus::memory::physical::PhysicalMemoryUtilization utilization;
};

[[nodiscard]] std::string_view policyName(
    const PageReplacementPolicyType policy)
{
    switch (policy)
    {
        case PageReplacementPolicyType::FIFO:
            return "FIFO";
        case PageReplacementPolicyType::LRU:
            return "LRU";
        case PageReplacementPolicyType::CLOCK:
            return "Clock";
        case PageReplacementPolicyType::OPTIMAL:
            return "Optimal";
    }

    throw std::logic_error("Unknown page-replacement policy.");
}

[[nodiscard]] PageReplacementPolicyFactory makePolicyFactory()
{
    PageReplacementPolicyFactory factory;
    const auto registerPolicy =
        [&factory](const PageReplacementPolicyType type,
                   PageReplacementPolicyFactory::PolicyCreator creator)
    {
        if (!factory.registerPolicy(type, std::move(creator)))
        {
            throw std::runtime_error("Failed to register a demo policy.");
        }
    };

    registerPolicy(
        PageReplacementPolicyType::FIFO,
        [] { return std::make_unique<
            emmus::algorithms::replacement::FIFOPageReplacementPolicy>(); });
    registerPolicy(
        PageReplacementPolicyType::LRU,
        [] { return std::make_unique<
            emmus::algorithms::replacement::LRUPageReplacementPolicy>(); });
    registerPolicy(
        PageReplacementPolicyType::CLOCK,
        [] { return std::make_unique<
            emmus::algorithms::replacement::ClockPageReplacementPolicy>(); });
    registerPolicy(
        PageReplacementPolicyType::OPTIMAL,
        [] { return std::make_unique<
            emmus::algorithms::replacement::OptimalPageReplacementPolicy>(); });

    return factory;
}

[[nodiscard]] std::vector<PageId> optimalReferenceSequence()
{
    std::vector<PageId> result;
    result.reserve(kReferenceTrace.size());
    for (const auto page : kReferenceTrace)
    {
        result.emplace_back(page);
    }
    return result;
}

[[nodiscard]] TraceResult runControlledTrace(
    const PageReplacementPolicyType policyType,
    SimulationActivityLog* const activityLog = nullptr)
{
    constexpr std::size_t frameCount = 3U;
    constexpr std::size_t pageCount = 5U;

    PageReplacementPolicyFactory factory = makePolicyFactory();
    auto policy = factory.create(policyType);
    if (!policy)
    {
        throw std::runtime_error("Could not create the selected demo policy.");
    }

    if (policyType == PageReplacementPolicyType::OPTIMAL)
    {
        auto* optimalPolicy =
            dynamic_cast<
                emmus::algorithms::replacement::
                    OptimalPageReplacementPolicy*>(policy.get());
        if (optimalPolicy == nullptr)
        {
            throw std::logic_error(
                "Optimal policy factory returned an unexpected implementation.");
        }
        optimalPolicy->setReferenceSequence(optimalReferenceSequence());
    }

    PageTable pageTable;
    PhysicalMemoryManager physicalMemory(frameCount);
    MemoryManagementUnit mmu(pageTable, physicalMemory, *policy, kPageSize);
    mmu.setActivityLog(activityLog);

    for (std::size_t pageNumber = 1U;
         pageNumber <= pageCount;
         ++pageNumber)
    {
        if (!mmu.registerPage(
                Page{PageId{pageNumber}, kProcessId},
                static_cast<std::uint64_t>(pageNumber)))
        {
            throw std::runtime_error("Failed to register a trace page.");
        }
    }

    for (std::size_t index = 0U; index < kReferenceTrace.size(); ++index)
    {
        const auto pageNumber = kReferenceTrace[index];
        const auto operation = index == 0U
            ? MemoryAccessOperation::Write
            : MemoryAccessOperation::Read;
        const auto address = pageNumber * kPageSize.value() + 32U;
        const MemoryAccess access{
            kProcessId,
            VirtualAddress{address},
            operation,
            emmus::memory::access::AccessSequenceNumber{index}};

        const auto result = mmu.access(access);
        if (!result.success())
        {
            throw std::runtime_error(
                "Controlled trace access failed: " + result.errorInformation());
        }
    }

    return {
        mmu.pageFaultCount(),
        mmu.pageReplacementCount(),
        mmu.dirtyEvictionCount(),
        policy->statistics().totalExecutionTime(),
        physicalMemory.snapshotUtilization()};
}

void printFrameSnapshot(
    const emmus::memory::physical::PhysicalMemoryUtilization& snapshot)
{
    std::cout << "  Physical frames: ";
    for (const auto& frame : snapshot.frames)
    {
        std::cout << "[F" << frame.frameId.value() << ':';
        if (frame.pageId.has_value())
        {
            std::cout << 'P' << frame.pageId->value();
        }
        else
        {
            std::cout << "free";
        }
        std::cout << "] ";
    }
    std::cout << "\n  Utilization: " << snapshot.allocatedFrames << '/'
              << snapshot.totalFrames << " ("
              << std::fixed << std::setprecision(1)
              << snapshot.utilizationPercent << "%)\n";
}

void printControlledTrace()
{
    std::cout << "\n1. Controlled MMU trace (3 frames, 5 pages)\n"
              << "   Reference string: ";
    for (const auto page : kReferenceTrace)
    {
        std::cout << page << ' ';
    }
    std::cout << "\n   One write to page 1; remaining references are reads.\n"
              << "   Policy     Faults  Replacements  Dirty evictions  Avg decision (us)\n";

    constexpr std::array policies{
        PageReplacementPolicyType::FIFO,
        PageReplacementPolicyType::LRU,
        PageReplacementPolicyType::CLOCK,
        PageReplacementPolicyType::OPTIMAL};

    TraceResult fifoResult{};
    TraceResult lruResult{};
    TraceResult optimalResult{};

    for (const auto policy : policies)
    {
        SimulationActivityLog activityLog;
        const auto result = runControlledTrace(policy, &activityLog);
        if (policy == PageReplacementPolicyType::FIFO)
        {
            fifoResult = result;
        }
        else if (policy == PageReplacementPolicyType::LRU)
        {
            lruResult = result;
        }
        else if (policy == PageReplacementPolicyType::OPTIMAL)
        {
            optimalResult = result;
        }

        const double averageMicroseconds =
            result.replacements == 0U
            ? 0.0
            : std::chrono::duration<double, std::micro>(
                  result.policyTime).count() /
                  static_cast<double>(result.replacements);

        std::cout << "   " << std::left << std::setw(10) << policyName(policy)
                  << std::right << std::setw(6) << result.faults
                  << std::setw(14) << result.replacements
                  << std::setw(17) << result.dirtyEvictions
                  << std::setw(19) << std::fixed << std::setprecision(3)
                  << averageMicroseconds << '\n';

        if (policy == PageReplacementPolicyType::FIFO)
        {
            printFrameSnapshot(result.utilization);
            for (const auto& event : activityLog.events())
            {
                if (event.type ==
                        emmus::simulation::activity::SimulationActivityType::PageFault
                    || event.type ==
                        emmus::simulation::activity::SimulationActivityType::PageReplacement
                    || event.type ==
                        emmus::simulation::activity::SimulationActivityType::DirtyEviction)
                {
                    std::cout << "  " << event.render() << '\n';
                }
            }
        }
    }

    if (fifoResult.faults != 9U
        || lruResult.faults != 10U
        || optimalResult.faults != 7U)
    {
        throw std::runtime_error(
            "Controlled trace did not match its verified FIFO/LRU/Optimal fault counts.");
    }

    std::cout
        << "   Verified reference outcomes: FIFO=9, LRU=10, Optimal=7 faults.\n"
        << "   Optimal receives the complete future trace in this direct MMU run.\n";
}

void printSimulationAndComparison()
{
    using emmus::simulation::SimulationProgress;

    std::cout << "\n2. Seeded multi-process simulation (locality workload)\n"
              << "   2 processes, 8 pages/process, 4 frames, 64 accesses, seed=1504.\n";

    const emmus::infrastructure::configuration::SimulationConfiguration
        configuration{
            kPageSize,
            FrameCount{4U},
            2U,
            8U,
            PageReplacementPolicyType::LRU,
            WorkloadType::Locality,
            64U,
            1504U};
    Simulation simulation(configuration);
    const auto result = simulation.run(
        [](const SimulationProgress& progress)
        {
            const auto accessCount =
                progress.executionStatistics.memoryAccessCount();
            if (accessCount % 16U == 0U)
            {
                std::cout << "   Progress: " << accessCount << '/'
                          << progress.totalAccessCount << " accesses, "
                          << progress.pageFaultStatistics.totalPageFaultCount()
                          << " faults, "
                          << progress.physicalMemoryUtilization.allocatedFrames
                          << '/' << progress.physicalMemoryUtilization.totalFrames
                          << " frames occupied.\n";
            }
        });

    if (!result.succeeded())
    {
        throw std::runtime_error(
            "Seeded simulation failed: " + result.diagnostic());
    }

    const auto& statistics =
        result.executionResult().statistics();
    if (statistics.memoryAccessCount() != 64U
        || statistics.failedAccessCount() != 0U)
    {
        throw std::runtime_error(
            "Seeded simulation did not complete all 64 accesses successfully.");
    }
    std::cout << "   Result: " << statistics.memoryAccessCount()
              << " accesses, " << statistics.pageFaultCount() << " faults ("
              << std::fixed << std::setprecision(1)
              << statistics.pageFaultRate() * 100.0 << "%), "
              << statistics.pageReplacementCount() << " replacements, "
              << statistics.dirtyEvictionCount() << " dirty evictions.\n"
              << "   Simulation elapsed: "
              << std::chrono::duration<double, std::milli>(
                     result.executionTime()).count()
              << " ms (machine/build dependent).\n";
    printFrameSnapshot(simulation.physicalMemoryManager().snapshotUtilization());

    std::cout << "   Activity events: " << result.activityLog().size()
              << "; representative events:\n";
    const auto& events = result.activityLog().events();
    std::size_t displayedEvents = 0U;
    for (const auto& event : events)
    {
        if (event.type ==
                emmus::simulation::activity::SimulationActivityType::PageFault
            || event.type ==
                emmus::simulation::activity::SimulationActivityType::PageReplacement
            || event.type ==
                emmus::simulation::activity::SimulationActivityType::DirtyEviction)
        {
            std::cout << "   " << event.render() << '\n';
            if (++displayedEvents == 8U)
            {
                break;
            }
        }
    }

    std::cout << "\n3. Same-seed policy comparisons (256 accesses per run)\n";
    const BenchmarkConfiguration benchmark{
        kPageSize,
        FrameCount{4U},
        1U,
        8U,
        256U,
        WorkloadType::Locality,
        1504U,
        PageReplacementPolicyType::FIFO};
    const SimulationComparison comparison{benchmark};
    const auto faultResults = comparison.run();
    const auto timingResults = comparison.runExecutionTimeComparison();

    std::cout << "   Policy     Faults  Fault rate  Replacements  Avg decision (us)  Total sim (ms)\n";
    constexpr std::array policies{
        PageReplacementPolicyType::FIFO,
        PageReplacementPolicyType::LRU,
        PageReplacementPolicyType::CLOCK,
        PageReplacementPolicyType::OPTIMAL};
    for (const auto policy : policies)
    {
        const auto faults = faultResults.resultFor(policy);
        const auto timing = timingResults.resultFor(policy);
        if (!faults.has_value() || !timing.has_value())
        {
            throw std::runtime_error(
                "Policy comparison omitted a supported policy.");
        }
        if (faults->totalAccessCount != 256U
            || faults->pageFaultCount > faults->totalAccessCount)
        {
            throw std::runtime_error(
                "Policy comparison returned invalid access/fault totals.");
        }

        std::cout << "   " << std::left << std::setw(10) << policyName(policy)
                  << std::right << std::setw(6) << faults->pageFaultCount
                  << std::setw(12) << std::fixed << std::setprecision(1)
                  << faults->faultRate * 100.0 << '%'
                  << std::setw(14) << timing->statistics.replacementCount()
                  << std::setw(19) << std::fixed << std::setprecision(3)
                  << timing->averageReplacementTimeMilliseconds() * 1000.0
                  << std::setw(16) << std::fixed << std::setprecision(3)
                  << timing->simulationTimeMilliseconds() << '\n';
    }
    std::cout
        << "   Comparisons hold workload configuration and seed constant.\n"
        << "   Timings are single-run observations, not performance claims.\n"
        << "   Caveat: the standard Simulation comparison does not feed Optimal\n"
        << "   a future trace; only the controlled trace above has true Optimal input.\n";
}

void printHelp()
{
    std::cout
        << "EMMUS portfolio demo\n\n"
        << "Usage: emmus-demo [--help]\n\n"
        << "Run the repeatable MMU trace, seeded simulation, policy comparisons,\n"
        << "utilization view, and event/statistics report. See docs/demo/us-1504-demonstration.md.\n";
}

} // namespace

int main(const int argc, char* argv[])
{
    if (argc > 1)
    {
        if (argc == 2 && std::string_view{argv[1]} == "--help")
        {
            printHelp();
            return 0;
        }
        std::cerr << "Unknown argument. Use --help for usage.\n";
        return 2;
    }

    try
    {
        std::cout
            << "============================================================\n"
            << "EMMUS | Enhanced Memory Management Unit Simulator\n"
            << "============================================================\n"
            << "Purpose: make virtual-page faults, frame allocation, replacement,\n"
            << "and policy trade-offs observable through a tested C++23 core.\n"
            << "Architecture: workload -> access executor -> MMU -> page table /\n"
            << "physical frames + policy -> statistics and typed activity log.\n"
            << "Scope: simulated mappings and counters; not an OS, TLB, or data emulator.\n";

        printControlledTrace();
        printSimulationAndComparison();

        std::cout
            << "\nEngineering evidence: ctest --preset debug --no-tests=error\n"
            << "Traceability: docs/requirements/requirements-traceability-matrix.md\n"
            << "============================================================\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "EMMUS demo failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
