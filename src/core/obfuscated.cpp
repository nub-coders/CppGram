#include "cppgram/obfuscated.hpp"
#include <algorithm>
#include <cstring>

namespace cppgram {

ObfuscatedCodec::ObfuscatedCodec(std::unique_ptr<ITransportCodec> inner_codec, TransportProtocol tag_proto)
    : inner_codec_(std::move(inner_codec)), tag_proto_(tag_proto) {
    generate_keys();
}

void ObfuscatedCodec::generate_keys() {
    bool valid = false;
    while (!valid) {
        init_payload_ = CryptoUtils::generate_random_bytes(64);
        if (init_payload_[0] == 0xef) continue;

        // Ensure not starting with HTTP verbs or protocol tags
        if (init_payload_[0] == 'G' && init_payload_[1] == 'E' && init_payload_[2] == 'T' && init_payload_[3] == ' ') continue;
        if (init_payload_[0] == 'P' && init_payload_[1] == 'O' && init_payload_[2] == 'S' && init_payload_[3] == 'T') continue;
        if (init_payload_[0] == 'H' && init_payload_[1] == 'E' && init_payload_[2] == 'A' && init_payload_[3] == 'D') continue;
        if (init_payload_[0] == 0xee && init_payload_[1] == 0xee && init_payload_[2] == 0xee && init_payload_[3] == 0xee) continue;

        uint32_t val4 = 0;
        std::memcpy(&val4, init_payload_.data() + 4, 4);
        if (val4 == 0) continue;

        valid = true;
    }

    uint32_t proto_tag = 0xeeeeeeee;
    if (tag_proto_ == TransportProtocol::Abridged) {
        proto_tag = 0xefefefef;
    } else if (tag_proto_ == TransportProtocol::PaddedIntermediate) {
        proto_tag = 0xdddddddd;
    }
    std::memcpy(init_payload_.data() + 56, &proto_tag, 4);

    enc_key_.assign(init_payload_.begin() + 8, init_payload_.begin() + 40);
    enc_iv_.assign(init_payload_.begin() + 40, init_payload_.begin() + 56);

    std::vector<uint8_t> rev = init_payload_;
    std::reverse(rev.begin(), rev.end());

    dec_key_.assign(rev.begin() + 8, rev.begin() + 40);
    dec_iv_.assign(rev.begin() + 40, rev.begin() + 56);

    // Initial handshake header
    handshake_header_ = init_payload_;
    std::vector<uint8_t> tail(init_payload_.begin() + 56, init_payload_.end());
    auto enc_tail = CryptoUtils::aes_ctr_encrypt(tail, enc_key_, enc_iv_);
    std::memcpy(handshake_header_.data() + 56, enc_tail.data(), 8);

    handshake_sent_ = false;
}

std::vector<uint8_t> ObfuscatedCodec::get_header() const {
    return handshake_header_;
}

std::vector<uint8_t> ObfuscatedCodec::encode_packet(const uint8_t* data, size_t length) {
    if (!inner_codec_) {
        return {};
    }

    auto framed = inner_codec_->encode_packet(data, length);
    auto enc = CryptoUtils::aes_ctr_encrypt(framed, enc_key_, enc_iv_);

    if (!handshake_sent_) {
        std::vector<uint8_t> res;
        res.reserve(handshake_header_.size() + enc.size());
        res.insert(res.end(), handshake_header_.begin(), handshake_header_.end());
        res.insert(res.end(), enc.begin(), enc.end());
        handshake_sent_ = true;
        return res;
    }

    return enc;
}

std::vector<std::vector<uint8_t>> ObfuscatedCodec::decode_packets(const uint8_t* data, size_t length) {
    if (!inner_codec_ || !data || length == 0) {
        return {};
    }

    std::vector<uint8_t> chunk(data, data + length);
    auto dec = CryptoUtils::aes_ctr_decrypt(chunk, dec_key_, dec_iv_);
    return inner_codec_->decode_packets(dec.data(), dec.size());
}

void ObfuscatedCodec::reset() {
    handshake_sent_ = false;
    if (inner_codec_) {
        inner_codec_->reset();
    }
}

// ---------------------------------------------------------------------------
// FakeTls
// ---------------------------------------------------------------------------

std::vector<uint8_t> FakeTls::create_client_hello(
    const std::string& sni_domain,
    const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> ch;

    // TLS Record Header: Type 0x16 (Handshake), Version 0x03, 0x01 (TLS 1.0)
    ch.push_back(0x16);
    ch.push_back(0x03);
    ch.push_back(0x01);

    // Placeholder for Record Length (2 bytes)
    size_t record_len_pos = ch.size();
    ch.push_back(0);
    ch.push_back(0);

    // Handshake Type 0x01 (ClientHello)
    ch.push_back(0x01);

    // Placeholder for Handshake Length (3 bytes)
    size_t hs_len_pos = ch.size();
    ch.push_back(0);
    ch.push_back(0);
    ch.push_back(0);

    // Protocol Version 0x03, 0x03 (TLS 1.2)
    ch.push_back(0x03);
    ch.push_back(0x03);

    // Random 32 bytes (embed part of payload or random)
    auto rand_bytes = CryptoUtils::generate_random_bytes(32);
    if (!payload.empty()) {
        size_t copy_len = std::min(payload.size(), size_t(32));
        std::memcpy(rand_bytes.data(), payload.data(), copy_len);
    }
    ch.insert(ch.end(), rand_bytes.begin(), rand_bytes.end());

    // Session ID length (32 bytes)
    ch.push_back(32);
    auto session_id = CryptoUtils::generate_random_bytes(32);
    ch.insert(ch.end(), session_id.begin(), session_id.end());

    // Cipher Suites Length (2 bytes) & Suites (TLS_AES_128_GCM_SHA256, TLS_CHACHA20_POLY1305_SHA256)
    ch.push_back(0x00);
    ch.push_back(0x04);
    ch.push_back(0x13); ch.push_back(0x01);
    ch.push_back(0x13); ch.push_back(0x03);

    // Compression Methods (0x01, 0x00)
    ch.push_back(0x01);
    ch.push_back(0x00);

    // Extensions Length placeholder (2 bytes)
    size_t ext_len_pos = ch.size();
    ch.push_back(0);
    ch.push_back(0);
    size_t ext_start = ch.size();

    // Extension: Server Name Indication (SNI, type 0x0000)
    ch.push_back(0x00); ch.push_back(0x00); // type
    uint16_t sni_ext_len = static_cast<uint16_t>(sni_domain.size() + 5);
    ch.push_back(static_cast<uint8_t>((sni_ext_len >> 8) & 0xFF));
    ch.push_back(static_cast<uint8_t>(sni_ext_len & 0xFF));

    // Server Name list length
    uint16_t sni_list_len = static_cast<uint16_t>(sni_domain.size() + 3);
    ch.push_back(static_cast<uint8_t>((sni_list_len >> 8) & 0xFF));
    ch.push_back(static_cast<uint8_t>(sni_list_len & 0xFF));

    // Host name type (0x00)
    ch.push_back(0x00);
    uint16_t host_len = static_cast<uint16_t>(sni_domain.size());
    ch.push_back(static_cast<uint8_t>((host_len >> 8) & 0xFF));
    ch.push_back(static_cast<uint8_t>(host_len & 0xFF));
    ch.insert(ch.end(), sni_domain.begin(), sni_domain.end());

    // Fill Extensions Length
    size_t total_ext_len = ch.size() - ext_start;
    ch[ext_len_pos] = static_cast<uint8_t>((total_ext_len >> 8) & 0xFF);
    ch[ext_len_pos + 1] = static_cast<uint8_t>(total_ext_len & 0xFF);

    // Fill Handshake Length (3 bytes)
    size_t total_hs_len = ch.size() - (hs_len_pos + 3);
    ch[hs_len_pos] = static_cast<uint8_t>((total_hs_len >> 16) & 0xFF);
    ch[hs_len_pos + 1] = static_cast<uint8_t>((total_hs_len >> 8) & 0xFF);
    ch[hs_len_pos + 2] = static_cast<uint8_t>(total_hs_len & 0xFF);

    // Fill Record Length (2 bytes)
    size_t total_rec_len = ch.size() - (record_len_pos + 2);
    ch[record_len_pos] = static_cast<uint8_t>((total_rec_len >> 8) & 0xFF);
    ch[record_len_pos + 1] = static_cast<uint8_t>(total_rec_len & 0xFF);

    return ch;
}

bool FakeTls::is_valid_tls_record(std::span<const uint8_t> data) {
    if (data.size() < 5) {
        return false;
    }
    // Record type Handshake (0x16), TLS 1.0 (0x03, 0x01) or TLS 1.2 (0x03, 0x03)
    if (data[0] != 0x16) return false;
    if (data[1] != 0x03) return false;
    if (data[2] != 0x01 && data[2] != 0x02 && data[2] != 0x03) return false;
    return true;
}

} // namespace cppgram
