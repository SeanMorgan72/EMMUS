#pragma once

#include <string>
#include <vector>

#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"

namespace emmus::infrastructure::configuration {

class SimulationConfigurationValidator final {
public:
    [[nodiscard]] static std::vector<std::string> validate(
        const SimulationConfiguration& configuration);

    [[nodiscard]] static bool isValid(
        const SimulationConfiguration& configuration);
};

} // namespace emmus::infrastructure::configuration