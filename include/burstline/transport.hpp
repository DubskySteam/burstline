#pragma once

#include "burstline/workload.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stop_token>
#include <string>

namespace burstline {

struct RequestResult {
    std::uint16_t status_code{};
    std::chrono::microseconds elapsed{};
    std::size_t bytes_received{};
    std::string error;

    [[nodiscard]] bool has_transport_error() const noexcept;
};

class IHttpSession {
public:
    virtual ~IHttpSession() = default;

    /// @brief Sends one HTTP request.
    /// @param stop_token Requests cancellation from the executor.
    /// @return The response or transport error.
    [[nodiscard]] virtual RequestResult perform(
        const RequestTemplate& request,
        std::stop_token stop_token) = 0;
};

class IHttpTransport {
public:
    virtual ~IHttpTransport() = default;

    /// @brief Creates an independent session for one execution worker.
    [[nodiscard]] virtual std::unique_ptr<IHttpSession> create_session() = 0;
};

}
