#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <span>

namespace cppgram {

class CryptoUtils {
public:
    static std::vector<uint8_t> compute_sha256(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> compute_sha256(std::span<const uint8_t> data);
    static std::vector<uint8_t> compute_sha1(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> compute_sha1(std::span<const uint8_t> data);

    static std::vector<uint8_t> aes_ige_encrypt(
        const std::vector<uint8_t>& plaintext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv);

    static std::vector<uint8_t> aes_ige_decrypt(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv);

    static std::vector<uint8_t> aes_ctr_encrypt(
        const std::vector<uint8_t>& plaintext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv);

    static std::vector<uint8_t> aes_ctr_decrypt(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv);

    static bool kdf_mtproto2(
        const std::vector<uint8_t>& auth_key,
        const std::vector<uint8_t>& msg_key,
        bool is_client,
        std::vector<uint8_t>& out_aes_key,
        std::vector<uint8_t>& out_aes_iv);

    static std::vector<uint8_t> compute_msg_key(
        const std::vector<uint8_t>& auth_key,
        const std::vector<uint8_t>& plaintext,
        bool is_client);

    static std::vector<uint8_t> generate_random_bytes(size_t count);
    
    static uint64_t compute_auth_key_id(const std::vector<uint8_t>& auth_key);
};

} // namespace cppgram
