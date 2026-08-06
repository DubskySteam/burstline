#pragma once

#include "burstline/transport.hpp"

#include <chrono>
#include <cstdint>
#include <stop_token>

namespace burstline {

struct ExecutionSummary {
    std::uint64_t requests_started{};
    std::uint64_t requests_finished{};
    std::uint64_t transport_errors{};
    std::uint64_t bytes_received{};
    std::chrono::milliseconds elapsed{};
};

class WorkloadExecutor final {
public:
    /// @brief Runs a validated workload with one session per configured connection.
    /// @pre `workload` passes `validate`.
    [[nodiscard]] ExecutionSummary run(
        const Workload& workload,
        IHttpTransport& transport,
        std::stop_token stop_token = {}) const;
};

}
