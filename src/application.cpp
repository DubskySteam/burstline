#include "burstline/application.hpp"
#include "burstline/command_line.hpp"

#include <iostream>
#include <string_view>

namespace burstline {
namespace {

constexpr std::string_view version{"0.1.0"};

void print_usage(std::ostream& output, std::string_view executable_name) {
    output << "Burstline " << version << "\n"
           << "High-throughput HTTP load tester with a terminal user interface.\n\n"
           << "Usage:\n"
           << "  " << executable_name << " --url URL [options]\n\n"
           << "Options:\n"
           << "  -u, --url URL        Absolute HTTP or HTTPS target URL.\n"
           << "  -X, --method METHOD  HTTP method: GET, HEAD, POST, PUT, PATCH, or DELETE.\n"
           << "  -c, --connections N  Concurrent connections (default: 1).\n"
           << "  -d, --duration TIME  Test duration: positive value ending in ms, s, or m (default: 10s).\n"
           << "      --rate N          Optional requests-per-second limit.\n"
           << "  -H, --header NAME:VALUE\n"
           << "                       Add a request header; may be repeated.\n"
           << "      --body TEXT       Request body.\n"
           << "  -h, --help     Show this help message.\n"
           << "  -V, --version  Show the Burstline version.\n";
}

void print_diagnostics(const std::vector<Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        std::cerr << (diagnostic.severity == DiagnosticSeverity::error ? "error" : "warning")
                  << ": " << diagnostic.message << '\n';
    }
}

}

int Application::run(const std::span<char* const> arguments) const {
    const std::string_view executable_name{
        arguments.empty() ? "burstline" : arguments.front()};

    if (arguments.size() == 1U) {
        print_usage(std::cout, executable_name);
        return 0;
    }

    const std::string_view command{arguments[1]};
    if (command == "--help" || command == "-h") {
        print_usage(std::cout, executable_name);
        return 0;
    }

    if (command == "--version" || command == "-V") {
        std::cout << "Burstline " << version << '\n';
        return 0;
    }

    const auto result = CommandLineParser{}.parse(arguments.subspan(1));
    print_diagnostics(result.diagnostics);
    if (!result.is_valid()) {
        std::cerr << '\n';
        print_usage(std::cerr, executable_name);
        return 2;
    }

    const auto& workload = *result.workload;
    std::cout << "Validated workload\n"
              << "  Target: " << workload.request.url << '\n'
              << "  Method: " << to_string(workload.request.method) << '\n'
              << "  Connections: " << workload.load.connections << '\n'
              << "  Duration: " << workload.load.duration.count() << "ms\n";
    if (workload.load.requests_per_second.has_value()) {
        std::cout << "  Rate limit: " << *workload.load.requests_per_second << " requests/s\n";
    }
    return 0;
}

}
