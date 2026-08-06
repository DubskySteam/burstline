#include "burstline/command_line.hpp"
#include "burstline/execution.hpp"
#include "burstline/workload.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <span>

namespace {

bool expect(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

class TestSession final : public burstline::IHttpSession {
public:
    explicit TestSession(std::atomic<std::uint64_t>& requests)
        : requests_{requests} {}

    burstline::RequestResult perform(
        const burstline::RequestTemplate&,
        std::stop_token) override {
        requests_.fetch_add(1, std::memory_order_relaxed);
        return {.status_code = 200, .bytes_received = 128};
    }

private:
    std::atomic<std::uint64_t>& requests_;
};

class TestTransport final : public burstline::IHttpTransport {
public:
    std::unique_ptr<burstline::IHttpSession> create_session() override {
        sessions_created.fetch_add(1, std::memory_order_relaxed);
        return std::make_unique<TestSession>(requests);
    }

    std::atomic<std::uint64_t> requests{};
    std::atomic<std::uint64_t> sessions_created{};
};

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

    burstline::Workload workload{};
    workload.request.url = "https://example.test/api";
    workload.load.connections = 2;
    workload.load.duration = std::chrono::milliseconds{20};
    workload.load.requests_per_second = 1'000;

    TestTransport transport;
    const auto summary = burstline::WorkloadExecutor{}.run(workload, transport);
    passed &= expect(transport.sessions_created == 2U, "one session should be created per connection");
    passed &= expect(summary.requests_started > 0U, "the executor should start requests");
    passed &= expect(summary.requests_started == summary.requests_finished,
                     "all started requests should finish");
    passed &= expect(summary.transport_errors == 0U, "successful requests should not be transport errors");
    passed &= expect(summary.bytes_received == summary.requests_finished * 128U,
                     "received bytes should be aggregated");

    return passed ? 0 : 1;
}
