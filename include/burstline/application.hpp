#pragma once

#include <span>

namespace burstline {

/// @brief Runs Burstline's command-line application.
class Application final {
public:
    /// @param arguments Process arguments; storage remains caller-owned.
    /// @return Zero on success; otherwise a process error code.
    [[nodiscard]] int run(std::span<char* const> arguments) const;
};

}
