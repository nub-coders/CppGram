#pragma once

#include "cppgram/crypto.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <span>

namespace cppgram {

struct MTProtoMessage {
    int64_t msg_id{0};
    int32_t seq_no{0};
    std::vector<uint8_t> payload;
};

class Session {
public:
    Session();
    explicit Session(uint64_t session_id);

    [[nodiscard]] uint64_t get_session_id() const noexcept { return session_id_; }
    void set_session_id(uint64_t id) noexcept { session_id_ = id; }

    [[nodiscard]] uint64_t get_server_salt() const noexcept { return server_salt_; }
    void set_server_salt(uint64_t salt) noexcept { server_salt_ = salt; }

    [[nodiscard]] const std::vector<uint8_t>& get_auth_key() const noexcept { return auth_key_; }
    void set_auth_key(const std::vector<uint8_t>& key);

    [[nodiscard]] uint64_t get_auth_key_id() const noexcept { return auth_key_id_; }

    [[nodiscard]] int64_t generate_msg_id();
    [[nodiscard]] int32_t generate_seq_no(bool is_content_related);

    void set_time_offset(int64_t offset_seconds) noexcept { time_offset_ = offset_seconds; }
    [[nodiscard]] int64_t get_time_offset() const noexcept { return time_offset_; }

    // Serialization & Encryption
    std::vector<uint8_t> pack_unencrypted_message(const std::vector<uint8_t>& payload);
    static bool unpack_unencrypted_message(
        std::span<const uint8_t> data,
        int64_t& out_msg_id,
        std::vector<uint8_t>& out_payload);

    std::vector<uint8_t> pack_encrypted_message(const std::vector<uint8_t>& payload, bool is_content_related);
    bool unpack_encrypted_message(
        std::span<const uint8_t> data,
        int64_t& out_msg_id,
        int32_t& out_seq_no,
        std::vector<uint8_t>& out_payload);

private:
    uint64_t session_id_{0};
    uint64_t server_salt_{0};
    std::vector<uint8_t> auth_key_;
    uint64_t auth_key_id_{0};
    int32_t content_seq_no_{0};
    int64_t last_msg_id_{0};
    int64_t time_offset_{0};
};

} // namespace cppgram
