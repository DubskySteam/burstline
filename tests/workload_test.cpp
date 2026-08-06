#include "burstline/command_line.hpp"
#include "burstline/workload.hpp"

#include <iostream>
#include <span>

namespace {

bool expect(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

}

int main() {
    bool passed = true;

    burstline::Workload invalid_workload{};
    invalid_workload.request.url = "ftp://example.test";
    invalid_workload.load.connections = 0;
    passed &= expect(burstline::contains_errors(burstline::validate(invalid_workload)),
                     "invalid workloads must produce errors");

    char url_option[] = "--url";
    char url[] = "https://example.test/api";
    char connections_option[] = "--connections";
    char connections[] = "8";
    char duration_option[] = "--duration";
    char duration[] = "1500ms";
    char* arguments[]{url_option, url, connections_option, connections, duration_option, duration};

    const auto result = burstline::CommandLineParser{}.parse(
        std::span<char* const>{arguments});
    passed &= expect(result.is_valid(), "a complete workload should parse successfully");
    passed &= expect(result.workload->load.connections == 8U, "connection count should be parsed");
    passed &= expect(result.workload->load.duration == std::chrono::milliseconds{1500},
                     "duration should be parsed");

    return passed ? 0 : 1;
}
