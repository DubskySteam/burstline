#include "burstline/workload.hpp"

#include <algorithm>

namespace burstline {

std::string_view to_string(const HttpMethod method) noexcept {
    switch (method) {
    case HttpMethod::get:
        return "GET";
    case HttpMethod::head:
        return "HEAD";
    case HttpMethod::post:
        return "POST";
    case HttpMethod::put:
        return "PUT";
    case HttpMethod::patch:
        return "PATCH";
    case HttpMethod::delete_:
        return "DELETE";
    }

    return "UNKNOWN";
}

std::vector<Diagnostic> validate(const Workload& workload) {
    std::vector<Diagnostic> diagnostics;

    const auto scheme_end = workload.request.url.find("://");
    const auto authority_start = scheme_end == std::string::npos ? 0U : scheme_end + 3U;
    const auto authority_end = workload.request.url.find_first_of("/?#", authority_start);
    if (scheme_end == std::string::npos ||
        (workload.request.url.compare(0, 7, "http://") != 0 &&
         workload.request.url.compare(0, 8, "https://") != 0) ||
        authority_start >= workload.request.url.size() || authority_end == authority_start) {
        diagnostics.push_back({DiagnosticSeverity::error,
                               "--url must be an absolute HTTP or HTTPS URL."});
    }

    if (workload.load.connections == 0U) {
        diagnostics.push_back({DiagnosticSeverity::error,
                               "--connections must be greater than zero."});
    }

    if (workload.load.duration <= std::chrono::milliseconds::zero()) {
        diagnostics.push_back({DiagnosticSeverity::error,
                               "--duration must be greater than zero."});
    }

    if (workload.load.requests_per_second.has_value() &&
        *workload.load.requests_per_second == 0U) {
        diagnostics.push_back({DiagnosticSeverity::error,
                               "--rate must be greater than zero when specified."});
    }

    for (const auto& header : workload.request.headers) {
        if (header.name.empty()) {
            diagnostics.push_back({DiagnosticSeverity::error,
                                   "HTTP header names must not be empty."});
        }
    }

    if ((workload.request.method == HttpMethod::get ||
         workload.request.method == HttpMethod::head) &&
        !workload.request.body.empty()) {
        diagnostics.push_back({DiagnosticSeverity::warning,
                               "A request body is unusual for GET and HEAD requests."});
    }

    return diagnostics;
}

bool contains_errors(const std::vector<Diagnostic>& diagnostics) noexcept {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& diagnostic) {
        return diagnostic.severity == DiagnosticSeverity::error;
    });
}

}