#include "cppgram/metrics.hpp"
#include <sstream>
#include <iomanip>

namespace cppgram {

void MetricsCollector::increment_messages_sent(uint64_t count) noexcept {
    messages_sent_.fetch_add(count, std::memory_order_relaxed);
}

void MetricsCollector::increment_messages_received(uint64_t count) noexcept {
    messages_received_.fetch_add(count, std::memory_order_relaxed);
}

void MetricsCollector::increment_rpc_calls(uint64_t count) noexcept {
    rpc_calls_.fetch_add(count, std::memory_order_relaxed);
}

void MetricsCollector::increment_errors(uint64_t count) noexcept {
    errors_.fetch_add(count, std::memory_order_relaxed);
}

void MetricsCollector::increment_reconnects(uint64_t count) noexcept {
    reconnects_.fetch_add(count, std::memory_order_relaxed);
}

void MetricsCollector::set_active_connections(int64_t count) noexcept {
    active_connections_.store(count, std::memory_order_relaxed);
}

void MetricsCollector::record_rpc_latency_ms(double ms) {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    total_latency_ms_ += ms;
    latency_samples_++;
}

uint64_t MetricsCollector::get_messages_sent() const noexcept {
    return messages_sent_.load(std::memory_order_relaxed);
}

uint64_t MetricsCollector::get_messages_received() const noexcept {
    return messages_received_.load(std::memory_order_relaxed);
}

uint64_t MetricsCollector::get_rpc_calls() const noexcept {
    return rpc_calls_.load(std::memory_order_relaxed);
}

uint64_t MetricsCollector::get_errors() const noexcept {
    return errors_.load(std::memory_order_relaxed);
}

uint64_t MetricsCollector::get_reconnects() const noexcept {
    return reconnects_.load(std::memory_order_relaxed);
}

int64_t MetricsCollector::get_active_connections() const noexcept {
    return active_connections_.load(std::memory_order_relaxed);
}

double MetricsCollector::get_avg_latency_ms() const {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    return latency_samples_ > 0 ? (total_latency_ms_ / static_cast<double>(latency_samples_)) : 0.0;
}

std::string MetricsCollector::to_prometheus_format() const {
    std::ostringstream ss;
    ss << "# HELP cppgram_messages_sent_total Total messages sent\n"
       << "# TYPE cppgram_messages_sent_total counter\n"
       << "cppgram_messages_sent_total " << get_messages_sent() << "\n\n";

    ss << "# HELP cppgram_messages_received_total Total messages received\n"
       << "# TYPE cppgram_messages_received_total counter\n"
       << "cppgram_messages_received_total " << get_messages_received() << "\n\n";

    ss << "# HELP cppgram_rpc_calls_total Total RPC requests executed\n"
       << "# TYPE cppgram_rpc_calls_total counter\n"
       << "cppgram_rpc_calls_total " << get_rpc_calls() << "\n\n";

    ss << "# HELP cppgram_errors_total Total errors encountered\n"
       << "# TYPE cppgram_errors_total counter\n"
       << "cppgram_errors_total " << get_errors() << "\n\n";

    ss << "# HELP cppgram_reconnects_total Total connection reconnect events\n"
       << "# TYPE cppgram_reconnects_total counter\n"
       << "cppgram_reconnects_total " << get_reconnects() << "\n\n";

    ss << "# HELP cppgram_active_connections Current active socket connections\n"
       << "# TYPE cppgram_active_connections gauge\n"
       << "cppgram_active_connections " << get_active_connections() << "\n\n";

    ss << "# HELP cppgram_rpc_avg_latency_ms Average RPC latency in milliseconds\n"
       << "# TYPE cppgram_rpc_avg_latency_ms gauge\n"
       << std::fixed << std::setprecision(3)
       << "cppgram_rpc_avg_latency_ms " << get_avg_latency_ms() << "\n";

    return ss.str();
}

void MetricsCollector::reset() noexcept {
    messages_sent_.store(0, std::memory_order_relaxed);
    messages_received_.store(0, std::memory_order_relaxed);
    rpc_calls_.store(0, std::memory_order_relaxed);
    errors_.store(0, std::memory_order_relaxed);
    reconnects_.store(0, std::memory_order_relaxed);
    active_connections_.store(0, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        total_latency_ms_ = 0.0;
        latency_samples_ = 0;
    }
}

} // namespace cppgram
