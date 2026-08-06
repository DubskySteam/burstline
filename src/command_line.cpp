#include "burstline/command_line.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>

namespace burstline {
namespace {

void add_error(CommandLineResult& result, std::string message) {
    result.diagnostics.push_back({DiagnosticSeverity::error, std::move(message)});
}

std::optional<std::string_view> option_value(
    const std::span<char* const> arguments,
    std::size_t& index,
    const std::string_view option,
    CommandLineResult& result) {
    if (++index >= arguments.size()) {
        add_error(result, std::string{option} + " requires a value.");
        return std::nullopt;
    }

    return arguments[index];
}

std::optional<std::uint32_t> parse_positive_integer(const std::string_view value) {
    std::uint32_t parsed{};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() || parsed == 0U) {
        return std::nullopt;
    }

    return parsed;
}

std::optional<std::chrono::milliseconds> parse_duration(const std::string_view value) {
    std::string_view number = value;
    std::uint64_t multiplier{};

    if (value.ends_with("ms")) {
        number.remove_suffix(2);
        multiplier = 1U;
    } else if (value.ends_with('s')) {
        number.remove_suffix(1);
        multiplier = 1'000U;
    } else if (value.ends_with('m')) {
        number.remove_suffix(1);
        multiplier = 60'000U;
    } else {
        return std::nullopt;
    }

    std::uint64_t parsed{};
    const auto [end, error] = std::from_chars(number.data(), number.data() + number.size(), parsed);
    if (number.empty() || error != std::errc{} || end != number.data() + number.size() ||
        parsed == 0U || parsed > static_cast<std::uint64_t>(
                                      std::numeric_limits<std::chrono::milliseconds::rep>::max()) /
                                      multiplier) {
        return std::nullopt;
    }

    return std::chrono::milliseconds{static_cast<std::chrono::milliseconds::rep>(parsed * multiplier)};
}

std::optional<HttpMethod> parse_method(const std::string_view value) {
    if (value == "GET") {
        return HttpMethod::get;
    }
    if (value == "HEAD") {
        return HttpMethod::head;
    }
    if (value == "POST") {
        return HttpMethod::post;
    }
    if (value == "PUT") {
        return HttpMethod::put;
    }
    if (value == "PATCH") {
        return HttpMethod::patch;
    }
    if (value == "DELETE") {
        return HttpMethod::delete_;
    }

    return std::nullopt;
}

}

bool CommandLineResult::is_valid() const noexcept {
    return workload.has_value() && !contains_errors(diagnostics);
}

CommandLineResult CommandLineParser::parse(const std::span<char* const> arguments) const {
    CommandLineResult result;
    result.workload.emplace();

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string_view option{arguments[index]};

        if (option == "--url" || option == "-u") {
            if (const auto value = option_value(arguments, index, option, result)) {
                result.workload->request.url = *value;
            }
        } else if (option == "--method" || option == "-X") {
            if (const auto value = option_value(arguments, index, option, result)) {
                const auto method = parse_method(*value);
                if (!method.has_value()) {
                    add_error(result, "--method must be one of GET, HEAD, POST, PUT, PATCH, or DELETE.");
                } else {
                    result.workload->request.method = *method;
                }
            }
        } else if (option == "--connections" || option == "-c") {
            if (const auto value = option_value(arguments, index, option, result)) {
                const auto connections = parse_positive_integer(*value);
                if (!connections.has_value()) {
                    add_error(result, "--connections must be a positive integer.");
                } else {
                    result.workload->load.connections = *connections;
                }
            }
        } else if (option == "--duration" || option == "-d") {
            if (const auto value = option_value(arguments, index, option, result)) {
                const auto duration = parse_duration(*value);
                if (!duration.has_value()) {
                    add_error(result, "--duration must be a positive value ending in ms, s, or m.");
                } else {
                    result.workload->load.duration = *duration;
                }
            }
        } else if (option == "--rate") {
            if (const auto value = option_value(arguments, index, option, result)) {
                const auto rate = parse_positive_integer(*value);
                if (!rate.has_value()) {
                    add_error(result, "--rate must be a positive integer.");
                } else {
                    result.workload->load.requests_per_second = *rate;
                }
            }
        } else if (option == "--header" || option == "-H") {
            if (const auto value = option_value(arguments, index, option, result)) {
                const auto separator = value->find(':');
                if (separator == std::string_view::npos || separator == 0U) {
                    add_error(result, "--header must use the form Name: Value.");
                } else {
                    result.workload->request.headers.push_back(
                        {std::string{value->substr(0, separator)},
                         std::string{value->substr(separator + 1)}});
                }
            }
        } else if (option == "--body") {
            if (const auto value = option_value(arguments, index, option, result)) {
                result.workload->request.body = *value;
            }
        } else {
            add_error(result, "unknown option '" + std::string{option} + "'.");
        }
    }

    const auto validation_diagnostics = validate(*result.workload);
    result.diagnostics.insert(
        result.diagnostics.end(), validation_diagnostics.begin(), validation_diagnostics.end());
    return result;
}

}
