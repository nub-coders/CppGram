#include "cppgram/session.hpp"
#include <chrono>
#include <cstring>
#include <stdexcept>

namespace cppgram {

Session::Session() {
    auto rand_bytes = CryptoUtils::generate_random_bytes(8);
    std::memcpy(&session_id_, rand_bytes.data(), 8);
}

Session::Session(uint64_t session_id) : session_id_(session_id) {}

void Session::set_auth_key(const std::vector<uint8_t>& key) {
    auth_key_ = key;
    if (auth_key_.size() == 256) {
        auth_key_id_ = CryptoUtils::compute_auth_key_id(auth_key_);
    } else {
        auth_key_id_ = 0;
    }
}

int64_t Session::generate_msg_id() {
    auto now = std::chrono::system_clock::now();
    auto epoch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    int64_t seconds = (epoch_ms / 1000) + time_offset_;
    int64_t fraction = (epoch_ms % 1000) * 4294967; // (1000 ms -> 2^32 fraction)

    int64_t msg_id = (seconds << 32) | (fraction & 0xFFFFFFFF);
    // Lower 2 bits must be 0 for client messages
    msg_id = msg_id & ~3LL;

    if (msg_id <= last_msg_id_) {
        msg_id = last_msg_id_ + 4;
    }
    last_msg_id_ = msg_id;
    return msg_id;
}

int32_t Session::generate_seq_no(bool is_content_related) {
    if (is_content_related) {
        int32_t seq = content_seq_no_ * 2 + 1;
        content_seq_no_++;
        return seq;
    }
    return content_seq_no_ * 2;
}

std::vector<uint8_t> Session::pack_unencrypted_message(const std::vector<uint8_t>& payload) {
    int64_t msg_id = generate_msg_id();
    uint32_t payload_len = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> packet(20 + payload.size());
    uint64_t auth_key_id = 0;

    std::memcpy(packet.data(), &auth_key_id, 8);
    std::memcpy(packet.data() + 8, &msg_id, 8);
    std::memcpy(packet.data() + 16, &payload_len, 4);
    if (!payload.empty()) {
        std::memcpy(packet.data() + 20, payload.data(), payload.size());
    }
    return packet;
}

bool Session::unpack_unencrypted_message(
    std::span<const uint8_t> data,
    int64_t& out_msg_id,
    std::vector<uint8_t>& out_payload) {
    if (data.size() < 20) {
        return false;
    }

    uint64_t auth_key_id = 0;
    std::memcpy(&auth_key_id, data.data(), 8);
    if (auth_key_id != 0) {
        return false;
    }

    std::memcpy(&out_msg_id, data.data() + 8, 8);
    uint32_t payload_len = 0;
    std::memcpy(&payload_len, data.data() + 16, 4);

    if (data.size() < 20 + payload_len) {
        return false;
    }

    out_payload.assign(data.data() + 20, data.data() + 20 + payload_len);
    return true;
}

std::vector<uint8_t> Session::pack_encrypted_message(const std::vector<uint8_t>& payload, bool is_content_related) {
    if (auth_key_.size() != 256) {
        throw std::runtime_error("AuthKey is not set or invalid for encrypted session");
    }

    int64_t msg_id = generate_msg_id();
    int32_t seq_no = generate_seq_no(is_content_related);
    uint32_t payload_len = static_cast<uint32_t>(payload.size());

    size_t header_len = 32; // salt (8) + session_id (8) + msg_id (8) + seq_no (4) + payload_len (4)
    size_t inner_len = header_len + payload.size();

    size_t pad_len = 16 - (inner_len % 16);
    if (pad_len < 12) {
        pad_len += 16;
    }

    std::vector<uint8_t> plaintext(inner_len + pad_len);
    std::memcpy(plaintext.data(), &server_salt_, 8);
    std::memcpy(plaintext.data() + 8, &session_id_, 8);
    std::memcpy(plaintext.data() + 16, &msg_id, 8);
    std::memcpy(plaintext.data() + 24, &seq_no, 4);
    std::memcpy(plaintext.data() + 28, &payload_len, 4);
    if (!payload.empty()) {
        std::memcpy(plaintext.data() + 32, payload.data(), payload.size());
    }

    auto random_padding = CryptoUtils::generate_random_bytes(pad_len);
    std::memcpy(plaintext.data() + inner_len, random_padding.data(), pad_len);

    auto msg_key = CryptoUtils::compute_msg_key(auth_key_, plaintext, true /* is_client */);

    std::vector<uint8_t> aes_key, aes_iv;
    CryptoUtils::kdf_mtproto2(auth_key_, msg_key, true, aes_key, aes_iv);

    auto ciphertext = CryptoUtils::aes_ige_encrypt(plaintext, aes_key, aes_iv);

    std::vector<uint8_t> result;
    result.reserve(8 + 16 + ciphertext.size());
    
    // auth_key_id (8 bytes)
    result.resize(8);
    std::memcpy(result.data(), &auth_key_id_, 8);

    // msg_key (16 bytes)
    result.insert(result.end(), msg_key.begin(), msg_key.end());

    // ciphertext
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

bool Session::unpack_encrypted_message(
    std::span<const uint8_t> data,
    int64_t& out_msg_id,
    int32_t& out_seq_no,
    std::vector<uint8_t>& out_payload) {
    if (data.size() < 24 + 32 || auth_key_.size() != 256) {
        return false;
    }

    uint64_t key_id = 0;
    std::memcpy(&key_id, data.data(), 8);
    if (key_id != auth_key_id_) {
        return false;
    }

    std::vector<uint8_t> msg_key(data.data() + 8, data.data() + 24);
    std::vector<uint8_t> ciphertext(data.data() + 24, data.data() + data.size());

    if (ciphertext.size() % 16 != 0) {
        return false;
    }

    std::vector<uint8_t> plaintext;
    bool decrypted = false;

    for (bool is_client : {true, false}) {
        std::vector<uint8_t> aes_key, aes_iv;
        if (!CryptoUtils::kdf_mtproto2(auth_key_, msg_key, is_client, aes_key, aes_iv)) {
            continue;
        }

        try {
            plaintext = CryptoUtils::aes_ige_decrypt(ciphertext, aes_key, aes_iv);
            auto expected_msg_key = CryptoUtils::compute_msg_key(auth_key_, plaintext, is_client);
            if (expected_msg_key == msg_key) {
                decrypted = true;
                break;
            }
        } catch (...) {
            continue;
        }
    }

    if (!decrypted || plaintext.size() < 32) {
        return false;
    }

    uint64_t salt = 0, session_id = 0;
    std::memcpy(&salt, plaintext.data(), 8);
    std::memcpy(&session_id, plaintext.data() + 8, 8);
    std::memcpy(&out_msg_id, plaintext.data() + 16, 8);
    std::memcpy(&out_seq_no, plaintext.data() + 24, 4);

    uint32_t payload_len = 0;
    std::memcpy(&payload_len, plaintext.data() + 28, 4);

    if (plaintext.size() < 32 + payload_len) {
        return false;
    }

    out_payload.assign(plaintext.data() + 32, plaintext.data() + 32 + payload_len);
    return true;
}

} // namespace cppgram
