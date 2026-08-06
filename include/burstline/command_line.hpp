#pragma once

#include "burstline/workload.hpp"

#include <optional>
#include <span>
#include <vector>

namespace burstline {

struct CommandLineResult {
    std::optional<Workload> workload;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool is_valid() const noexcept;
};

/// @brief Parses workload options after the executable name.
class CommandLineParser final {
public:
    /// @param arguments Arguments to parse; storage remains caller-owned.
    /// @return A parsed workload and any diagnostics.
    [[nodiscard]] CommandLineResult parse(std::span<char* const> arguments) const;
};

}
