#include "cppgram/crypto.hpp"
#include "cppgram/transport.hpp"
#include "cppgram/tl.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace cppgram;

void bench_crypto_ige() {
    std::cout << "--- Benchmarking AES-256-IGE Throughput ---\n";
    std::vector<uint8_t> key(32, 0x42);
    std::vector<uint8_t> iv(32, 0x24);
    size_t chunk_size = 64 * 1024; // 64 KB
    std::vector<uint8_t> plaintext(chunk_size, 0x55);

    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto ciphertext = CryptoUtils::aes_ige_encrypt(plaintext, key, iv);
        auto decrypted = CryptoUtils::aes_ige_decrypt(ciphertext, key, iv);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed_sec = std::chrono::duration<double>(end - start).count();
    double total_mb = (static_cast<double>(chunk_size * 2 * iterations)) / (1024.0 * 1024.0);
    std::cout << "  Processed: " << total_mb << " MB in " << elapsed_sec << " s\n";
    std::cout << "  Throughput: " << (total_mb / elapsed_sec) << " MB/s\n\n";
}

void bench_transport_framing() {
    std::cout << "--- Benchmarking MTProto Framing Codecs ---\n";
    std::vector<uint8_t> payload(1024, 0xAA);
    const int iterations = 50000;

    for (auto proto : {TransportProtocol::Abridged, TransportProtocol::Intermediate, TransportProtocol::Full}) {
        auto codec = create_transport_codec(proto);
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            auto frame = codec->encode_packet(payload);
            auto pkts = codec->decode_packets(frame);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_sec = std::chrono::duration<double>(end - start).count();
        double ops_sec = static_cast<double>(iterations) / elapsed_sec;
        std::string proto_name = (proto == TransportProtocol::Abridged) ? "Abridged" :
                                 (proto == TransportProtocol::Intermediate) ? "Intermediate" : "Full (CRC32)";
        std::cout << "  Codec " << std::left << std::setw(15) << proto_name
                  << ": " << std::fixed << std::setprecision(0) << ops_sec << " ops/sec\n";
    }
    std::cout << "\n";
}

void bench_tl_serialization() {
    std::cout << "--- Benchmarking Type Language (TL) Codec ---\n";
    const int iterations = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        TLWriter writer;
        writer.write_uint32(TL_PING);
        writer.write_int64(123456789012345LL);
        writer.write_string("benchmark_sample_string");
        writer.write_double(3.1415926535);

        TLReader reader(writer.data());
        uint32_t ctor = reader.read_uint32();
        int64_t ping_id = reader.read_int64();
        std::string str = reader.read_string();
        double d = reader.read_double();
        (void)ctor; (void)ping_id; (void)str; (void)d;
    }
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end - start).count();
    double ops_sec = static_cast<double>(iterations) / elapsed_sec;
    std::cout << "  TL Serialization/Deserialization: "
              << std::fixed << std::setprecision(0) << ops_sec << " roundtrips/sec\n\n";
}

int main() {
    std::cout << "=======================================================\n";
    std::cout << "          CppGram Engine Performance Benchmarks        \n";
    std::cout << "=======================================================\n\n";

    bench_crypto_ige();
    bench_transport_framing();
    bench_tl_serialization();

    std::cout << "=======================================================\n";
    std::cout << "                Benchmarks Finished                    \n";
    std::cout << "=======================================================\n";
    return 0;
}
