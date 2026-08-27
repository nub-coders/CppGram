#pragma once

#include <cstdint>
#include <string>
#include <atomic>
#include <mutex>
#include <vector>

namespace cppgram {

class MetricsCollector {
public:
    MetricsCollector() = default;

    void increment_messages_sent(uint64_t count = 1) noexcept;
    void increment_messages_received(uint64_t count = 1) noexcept;
    void increment_rpc_calls(uint64_t count = 1) noexcept;
    void increment_errors(uint64_t count = 1) noexcept;
    void increment_reconnects(uint64_t count = 1) noexcept;
    void set_active_connections(int64_t count) noexcept;
    void record_rpc_latency_ms(double ms);

    [[nodiscard]] uint64_t get_messages_sent() const noexcept;
    [[nodiscard]] uint64_t get_messages_received() const noexcept;
    [[nodiscard]] uint64_t get_rpc_calls() const noexcept;
    [[nodiscard]] uint64_t get_errors() const noexcept;
    [[nodiscard]] uint64_t get_reconnects() const noexcept;
    [[nodiscard]] int64_t get_active_connections() const noexcept;
    [[nodiscard]] double get_avg_latency_ms() const;

    [[nodiscard]] std::string to_prometheus_format() const;
    void reset() noexcept;

private:
    std::atomic<uint64_t> messages_sent_{0};
    std::atomic<uint64_t> messages_received_{0};
    std::atomic<uint64_t> rpc_calls_{0};
    std::atomic<uint64_t> errors_{0};
    std::atomic<uint64_t> reconnects_{0};
    std::atomic<int64_t> active_connections_{0};

    mutable std::mutex latency_mutex_;
    double total_latency_ms_{0.0};
    uint64_t latency_samples_{0};
};

} // namespace cppgram
