#include "cppgram/crypto.hpp"
#include "cppgram/network.hpp"
#include "cppgram/session.hpp"
#include "cppgram/cli.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace cppgram;

static void print_hex(const std::string& label, const std::vector<uint8_t>& data, size_t max_bytes = 16) {
    std::cout << "  " << label << " (" << data.size() << " bytes): ";
    size_t count = std::min(data.size(), max_bytes);
    for (size_t i = 0; i < count; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    if (data.size() > max_bytes) {
        std::cout << "...";
    }
    std::cout << std::dec << "\n";
}

int main() {
    std::cout << "=======================================================\n";
    std::cout << "      CppGram Version 0.5 Feature Showcase Demo        \n";
    std::cout << "=======================================================\n\n";

    // 1. MTProto 2.0 Cryptographic Primitives
    std::cout << "[1] MTProto 2.0 Cryptographic Engine (AES-256-IGE & KDF)\n";
    std::vector<uint8_t> auth_key(256);
    for (size_t i = 0; i < 256; ++i) {
        auth_key[i] = static_cast<uint8_t>((i * 13 + 37) & 0xFF);
    }
    uint64_t key_id = CryptoUtils::compute_auth_key_id(auth_key);
    std::cout << "  Calculated AuthKey ID: 0x" << std::hex << key_id << std::dec << "\n";

    std::string sample_text = "Modern Telegram MTProto 2.0 C++20 Framework Payload!";
    std::vector<uint8_t> plaintext(sample_text.begin(), sample_text.end());
    // Pad to multiple of 16 for AES-IGE demonstration
    while (plaintext.size() % 16 != 0) {
        plaintext.push_back(0);
    }

    std::vector<uint8_t> ige_key(32, 0xAA);
    std::vector<uint8_t> ige_iv(32, 0xBB);
    auto ciphertext = CryptoUtils::aes_ige_encrypt(plaintext, ige_key, ige_iv);
    auto decrypted = CryptoUtils::aes_ige_decrypt(ciphertext, ige_key, ige_iv);
    assert(decrypted == plaintext);

    print_hex("Plaintext", plaintext);
    print_hex("AES-IGE Ciphertext", ciphertext);
    print_hex("Decrypted Payload", decrypted);
    std::cout << "  AES-256-IGE encryption and decryption verified successfully.\n\n";

    // 2. Datacenter Manager & Routing
    std::cout << "[2] Datacenter Manager & DC Routing\n";
    DatacenterManager dc_mgr;
    auto primary_dc = dc_mgr.get_primary_dc();
    if (primary_dc) {
        std::cout << "  Primary Production DC: " << primary_dc->name << "\n";
        std::cout << "  Endpoint: " << primary_dc->ip_v4 << ":" << primary_dc->port << "\n";
    }

    auto all_dcs = dc_mgr.get_all_dcs();
    std::cout << "  Registered Production DataCenters (" << all_dcs.size() << " total):\n";
    for (const auto& dc : all_dcs) {
        std::cout << "    - DC " << dc.id << ": " << dc.ip_v4 << ":" << dc.port << " [" << dc.name << "]\n";
    }
    std::cout << "\n";

    // 3. MTProto Session State & Envelope Packing
    std::cout << "[3] MTProto Session State & Secure Envelope Processing\n";
    Session client_session(0x1122334455667788ULL);
    client_session.set_auth_key(auth_key);
    client_session.set_server_salt(0x99AABBCCDDEEFF00ULL);

    int64_t msg_id_1 = client_session.generate_msg_id();
    int64_t msg_id_2 = client_session.generate_msg_id();
    std::cout << "  Monotonic Msg ID 1: " << msg_id_1 << "\n";
    std::cout << "  Monotonic Msg ID 2: " << msg_id_2 << " (greater than msg_id_1: " << (msg_id_2 > msg_id_1 ? "YES" : "NO") << ")\n";

    std::vector<uint8_t> payload = {'c', 'p', 'p', 'g', 'r', 'a', 'm', '_', 'r', 'p', 'c'};
    auto encrypted_frame = client_session.pack_encrypted_message(payload, true);
    std::cout << "  Encrypted MTProto Frame Size: " << encrypted_frame.size() << " bytes\n";

    Session server_session;
    server_session.set_auth_key(auth_key);
    int64_t rx_msg_id = 0;
    int32_t rx_seq_no = 0;
    std::vector<uint8_t> rx_payload;
    bool unpacked_ok = server_session.unpack_encrypted_message(encrypted_frame, rx_msg_id, rx_seq_no, rx_payload);
    assert(unpacked_ok && rx_payload == payload);
    std::cout << "  Unpacked Payload: " << std::string(rx_payload.begin(), rx_payload.end()) << "\n";
    std::cout << "  Unpacked Msg ID: " << rx_msg_id << ", Seq No: " << rx_seq_no << "\n\n";

    // 4. Interactive CLI Framework
    std::cout << "[4] Interactive CLI Shell Dispatcher\n";
    InteractiveCLI cli;
    cli.register_command(
        "/ping",
        "Ping datacenter endpoint",
        "/ping [dc_id]",
        [](const CommandContext& ctx) {
            std::string dc = ctx.args.empty() ? "2" : ctx.args[0];
            ctx.out << "  Pinging DC " << dc << "... Pong! (latency: 18ms)\n";
        });

    cli.register_command(
        "/info",
        "Display client session information",
        "/info",
        [&](const CommandContext& ctx) {
            ctx.out << "  Session ID: 0x" << std::hex << client_session.get_session_id() << std::dec << "\n";
        });

    std::cout << "  Simulating command execution:\n";
    cli.execute_line("/ping 4");
    cli.execute_line("/info");

    std::cout << "\n=======================================================\n";
    std::cout << "      All Version 0.5 Demonstrations Completed!       \n";
    std::cout << "=======================================================\n";

    return 0;
}
