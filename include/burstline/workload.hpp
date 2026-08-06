#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace burstline {

enum class HttpMethod {
    get,
    head,
    post,
    put,
    patch,
    delete_,
};

[[nodiscard]] std::string_view to_string(HttpMethod method) noexcept;

struct Header {
    std::string name;
    std::string value;
};

struct RequestTemplate {
    std::string url;
    HttpMethod method{HttpMethod::get};
    std::vector<Header> headers;
    std::string body;
};

struct LoadProfile {
    std::uint32_t connections{1};
    std::chrono::milliseconds duration{std::chrono::seconds{10}};
    std::optional<std::uint32_t> requests_per_second;
};

/// @brief An HTTP workload independent of transport and presentation.
struct Workload {
    RequestTemplate request;
    LoadProfile load;
};

enum class DiagnosticSeverity {
    warning,
    error,
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string message;
};

/// @brief Validates a workload before execution.
/// @return Diagnostics; errors reject execution.
[[nodiscard]] std::vector<Diagnostic> validate(const Workload& workload);

[[nodiscard]] bool contains_errors(const std::vector<Diagnostic>& diagnostics) noexcept;

}
