#include "burstline/execution.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>

namespace burstline {
namespace {

struct AtomicSummary {
    std::atomic<std::uint64_t> requests_started{};
    std::atomic<std::uint64_t> requests_finished{};
    std::atomic<std::uint64_t> transport_errors{};
    std::atomic<std::uint64_t> bytes_received{};
};

RequestResult perform_request(
    IHttpSession& session,
    const RequestTemplate& request,
    const std::stop_token stop_token) {
    try {
        return session.perform(request, stop_token);
    } catch (const std::exception& exception) {
        return {.error = exception.what()};
    } catch (...) {
        return {.error = "unknown transport failure"};
    }
}

} 

ExecutionSummary WorkloadExecutor::run(
    const Workload& workload,
    IHttpTransport& transport,
    const std::stop_token stop_token) const {
    if (contains_errors(validate(workload))) {
        throw std::invalid_argument{"workload must pass validation before execution"};
    }

    std::vector<std::unique_ptr<IHttpSession>> sessions;
    sessions.reserve(workload.load.connections);
    for (std::uint32_t index = 0; index < workload.load.connections; ++index) {
        auto session = transport.create_session();
        if (!session) {
            throw std::runtime_error{"transport returned a null session"};
        }
        sessions.push_back(std::move(session));
    }

    AtomicSummary totals;
    const auto started_at = std::chrono::steady_clock::now();
    const auto deadline = started_at + workload.load.duration;
    const auto request_interval = workload.load.requests_per_second.has_value()
        ? std::max(
              std::chrono::nanoseconds{1},
              std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds{1}) /
                  *workload.load.requests_per_second)
        : std::chrono::nanoseconds::zero();
    std::atomic<std::uint64_t> next_slot{};

    {
        std::vector<std::jthread> workers;
        workers.reserve(sessions.size());

        for (const auto& session : sessions) {
            workers.emplace_back([&workload,
                                  &totals,
                                  deadline,
                                  request_interval,
                                  started_at,
                                  &next_slot,
                                  session = session.get(),
                                  stop_token] {
                while (!stop_token.stop_requested() && std::chrono::steady_clock::now() < deadline) {
                    if (request_interval != std::chrono::nanoseconds::zero()) {
                        const auto slot = next_slot.fetch_add(1, std::memory_order_relaxed);
                        const auto scheduled_at = started_at + request_interval * slot;
                        if (scheduled_at >= deadline) {
                            return;
                        }
                        std::this_thread::sleep_until(scheduled_at);
                        if (stop_token.stop_requested()) {
                            return;
                        }
                    }

                    if (std::chrono::steady_clock::now() >= deadline) {
                        return;
                    }

                    totals.requests_started.fetch_add(1, std::memory_order_relaxed);
                    const auto result = perform_request(*session, workload.request, stop_token);
                    totals.requests_finished.fetch_add(1, std::memory_order_relaxed);
                    totals.bytes_received.fetch_add(result.bytes_received, std::memory_order_relaxed);
                    if (result.has_transport_error()) {
                        totals.transport_errors.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }
    }

    return {
        .requests_started = totals.requests_started.load(std::memory_order_relaxed),
        .requests_finished = totals.requests_finished.load(std::memory_order_relaxed),
        .transport_errors = totals.transport_errors.load(std::memory_order_relaxed),
        .bytes_received = totals.bytes_received.load(std::memory_order_relaxed),
        .elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started_at),
    };
}

}
