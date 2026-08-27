#pragma once

#include "cppgram/transport.hpp"
#include "cppgram/crypto.hpp"
#include <vector>
#include <memory>
#include <string>
#include <array>
#include <span>

namespace cppgram {

class ObfuscatedCodec : public ITransportCodec {
public:
    using ITransportCodec::encode_packet;
    using ITransportCodec::decode_packets;

    explicit ObfuscatedCodec(std::unique_ptr<ITransportCodec> inner_codec, TransportProtocol tag_proto = TransportProtocol::Intermediate);
    ~ObfuscatedCodec() override = default;

    std::vector<uint8_t> get_header() const override;
    std::vector<uint8_t> encode_packet(const uint8_t* data, size_t length) override;
    std::vector<std::vector<uint8_t>> decode_packets(const uint8_t* data, size_t length) override;
    void reset() override;

    [[nodiscard]] const std::vector<uint8_t>& get_init_payload() const noexcept { return init_payload_; }
    [[nodiscard]] bool is_handshake_sent() const noexcept { return handshake_sent_; }

private:
    void generate_keys();

    std::unique_ptr<ITransportCodec> inner_codec_;
    TransportProtocol tag_proto_;
    std::vector<uint8_t> init_payload_;
    std::vector<uint8_t> enc_key_;
    std::vector<uint8_t> enc_iv_;
    std::vector<uint8_t> dec_key_;
    std::vector<uint8_t> dec_iv_;
    std::vector<uint8_t> handshake_header_;
    bool handshake_sent_{false};
};

class FakeTls {
public:
    static std::vector<uint8_t> create_client_hello(
        const std::string& sni_domain,
        const std::vector<uint8_t>& payload);

    static bool is_valid_tls_record(std::span<const uint8_t> data);
};

} // namespace cppgram
