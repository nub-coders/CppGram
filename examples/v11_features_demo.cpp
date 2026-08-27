#include "cppgram/obfuscated.hpp"
#include "cppgram/account_pool.hpp"
#include "cppgram/metrics.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace cppgram;

int main() {
    std::cout << "=======================================================\n";
    std::cout << "      CppGram Version 1.1 Enterprise Showcase Demo     \n";
    std::cout << "=======================================================\n\n";

    // 1. MTProto Obfuscated Transport Codec
    std::cout << "[1] MTProto Obfuscated Transport Codec (Anti-DPI / Anti-Censorship)\n";
    auto base_codec = create_transport_codec(TransportProtocol::Intermediate);
    ObfuscatedCodec obf_codec(std::move(base_codec), TransportProtocol::Intermediate);

    auto handshake = obf_codec.get_header();
    std::cout << "  Obfuscated Handshake Header Size: " << handshake.size() << " bytes\n";
    assert(handshake.size() == 64);
    assert(handshake[0] != 0xef); // Validated anti-heuristic header

    std::vector<uint8_t> rpc_data = {'r', 'e', 'q', '_', 'p', 'q', '_', 'm', 'u', 'l', 't', 'i'};
    auto framed_pkt = obf_codec.encode_packet(rpc_data);
    std::cout << "  First Encoded Packet (Handshake + Encrypted): " << framed_pkt.size() << " bytes\n";
    assert(framed_pkt.size() > 64);
    assert(obf_codec.is_handshake_sent());

    // 2. Fake-TLS Protocol Framing
    std::cout << "\n[2] Fake-TLS Protocol Framing (Domain Fronting)\n";
    std::string sni = "cloudflare.com";
    auto client_hello = FakeTls::create_client_hello(sni, handshake);
    std::cout << "  Generated TLS 1.3 ClientHello Size: " << client_hello.size() << " bytes\n";
    std::cout << "  SNI Target Domain: " << sni << "\n";
    assert(FakeTls::is_valid_tls_record(client_hello));
    std::cout << "  TLS Record Validation: PASSED (Type 0x16 Handshake Record)\n";

    // 3. Multi-Account Pool Orchestration
    std::cout << "\n[3] Multi-Account Session Pool Orchestrator\n";
    AccountPool pool;
    auto client1 = std::make_shared<Client>();
    auto client2 = std::make_shared<Client>();
    auto client3 = std::make_shared<Client>();

    pool.add_account("bot_alpha", client1);
    pool.add_account("bot_beta", client2);
    pool.add_account("bot_gamma", client3);

    std::cout << "  Configured Pool Size: " << pool.size() << " accounts\n";
    assert(pool.size() == 3);
    assert(pool.has_account("bot_alpha"));
    assert(pool.has_account("bot_beta"));
    assert(pool.has_account("bot_gamma"));

    // Verify round-robin selection
    auto acc1 = pool.get_next_account();
    auto acc2 = pool.get_next_account();
    auto acc3 = pool.get_next_account();
    auto acc4 = pool.get_next_account();
    assert(acc1 == client1);
    assert(acc2 == client2);
    assert(acc3 == client3);
    assert(acc4 == client1); // Wraps around
    std::cout << "  Round-Robin Load Balancing: VERIFIED (alpha -> beta -> gamma -> alpha)\n";

    // 4. Real-Time Telemetry & Prometheus Metrics
    std::cout << "\n[4] Real-Time Telemetry & Prometheus Metrics Exporter\n";
    MetricsCollector metrics;
    metrics.increment_messages_sent(1540);
    metrics.increment_messages_received(3280);
    metrics.increment_rpc_calls(4820);
    metrics.increment_reconnects(2);
    metrics.set_active_connections(5);
    metrics.record_rpc_latency_ms(18.5);
    metrics.record_rpc_latency_ms(24.2);
    metrics.record_rpc_latency_ms(19.8);

    std::cout << "  Prometheus Exporter Output:\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << metrics.to_prometheus_format();
    std::cout << "-------------------------------------------------------\n";

    std::cout << "\n=======================================================\n";
    std::cout << "     Version 1.1 Enterprise Capabilities Validated!    \n";
    std::cout << "=======================================================\n";
    return 0;
}
