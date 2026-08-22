#include "cppgram/crypto.hpp"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>
#include <array>

namespace cppgram {

std::vector<uint8_t> CryptoUtils::compute_sha256(const std::vector<uint8_t>& data) {
    return compute_sha256(std::span<const uint8_t>(data.data(), data.size()));
}

std::vector<uint8_t> CryptoUtils::compute_sha256(std::span<const uint8_t> data) {
    std::vector<uint8_t> hash(32);
    unsigned int len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx, data.data(), data.size());
        EVP_DigestFinal_ex(ctx, hash.data(), &len);
        EVP_MD_CTX_free(ctx);
    }
    return hash;
}

std::vector<uint8_t> CryptoUtils::compute_sha1(const std::vector<uint8_t>& data) {
    return compute_sha1(std::span<const uint8_t>(data.data(), data.size()));
}

std::vector<uint8_t> CryptoUtils::compute_sha1(std::span<const uint8_t> data) {
    std::vector<uint8_t> hash(20);
    unsigned int len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
        EVP_DigestUpdate(ctx, data.data(), data.size());
        EVP_DigestFinal_ex(ctx, hash.data(), &len);
        EVP_MD_CTX_free(ctx);
    }
    return hash;
}

std::vector<uint8_t> CryptoUtils::aes_ige_encrypt(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv) {
    if (key.size() != 32 || iv.size() != 32) {
        throw std::invalid_argument("AES-256-IGE requires 32-byte key and 32-byte IV");
    }
    if (plaintext.size() % 16 != 0) {
        throw std::invalid_argument("Plaintext length must be a multiple of 16 bytes for AES-IGE");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_CIPHER_CTX");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, key.data(), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize AES-256-ECB encryption for IGE");
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    std::vector<uint8_t> ciphertext(plaintext.size());
    std::array<uint8_t, 16> iv1; // c_0
    std::array<uint8_t, 16> iv2; // p_0
    std::memcpy(iv1.data(), iv.data(), 16);
    std::memcpy(iv2.data(), iv.data() + 16, 16);

    std::array<uint8_t, 16> block_in;
    std::array<uint8_t, 16> block_out;
    int out_len = 0;

    size_t blocks = plaintext.size() / 16;
    for (size_t b = 0; b < blocks; ++b) {
        const uint8_t* p_curr = plaintext.data() + b * 16;
        uint8_t* c_curr = ciphertext.data() + b * 16;

        for (int i = 0; i < 16; ++i) {
            block_in[i] = p_curr[i] ^ iv1[i];
        }

        if (EVP_EncryptUpdate(ctx, block_out.data(), &out_len, block_in.data(), 16) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("EVP_EncryptUpdate failed during AES-IGE");
        }

        for (int i = 0; i < 16; ++i) {
            c_curr[i] = block_out[i] ^ iv2[i];
            iv1[i] = c_curr[i];
            iv2[i] = p_curr[i];
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

std::vector<uint8_t> CryptoUtils::aes_ige_decrypt(
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv) {
    if (key.size() != 32 || iv.size() != 32) {
        throw std::invalid_argument("AES-256-IGE requires 32-byte key and 32-byte IV");
    }
    if (ciphertext.size() % 16 != 0) {
        throw std::invalid_argument("Ciphertext length must be a multiple of 16 bytes for AES-IGE");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_CIPHER_CTX");
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, key.data(), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize AES-256-ECB decryption for IGE");
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    std::vector<uint8_t> plaintext(ciphertext.size());
    std::array<uint8_t, 16> iv1; // c_0
    std::array<uint8_t, 16> iv2; // p_0
    std::memcpy(iv1.data(), iv.data(), 16);
    std::memcpy(iv2.data(), iv.data() + 16, 16);

    std::array<uint8_t, 16> block_in;
    std::array<uint8_t, 16> block_out;
    int out_len = 0;

    size_t blocks = ciphertext.size() / 16;
    for (size_t b = 0; b < blocks; ++b) {
        const uint8_t* c_curr = ciphertext.data() + b * 16;
        uint8_t* p_curr = plaintext.data() + b * 16;

        for (int i = 0; i < 16; ++i) {
            block_in[i] = c_curr[i] ^ iv2[i];
        }

        if (EVP_DecryptUpdate(ctx, block_out.data(), &out_len, block_in.data(), 16) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("EVP_DecryptUpdate failed during AES-IGE");
        }

        for (int i = 0; i < 16; ++i) {
            p_curr[i] = block_out[i] ^ iv1[i];
            iv1[i] = c_curr[i];
            iv2[i] = p_curr[i];
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

std::vector<uint8_t> CryptoUtils::aes_ctr_encrypt(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv) {
    if (key.size() != 32 || iv.size() < 16) {
        throw std::invalid_argument("AES-256-CTR requires 32-byte key and at least 16-byte IV");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_CIPHER_CTX");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize AES-256-CTR");
    }

    std::vector<uint8_t> ciphertext(plaintext.size());
    int out_len = 0;
    if (!plaintext.empty()) {
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("EVP_EncryptUpdate failed during AES-CTR");
        }
    }

    int final_len = 0;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &final_len);
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

std::vector<uint8_t> CryptoUtils::aes_ctr_decrypt(
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv) {
    return aes_ctr_encrypt(ciphertext, key, iv);
}

bool CryptoUtils::kdf_mtproto2(
    const std::vector<uint8_t>& auth_key,
    const std::vector<uint8_t>& msg_key,
    bool is_client,
    std::vector<uint8_t>& out_aes_key,
    std::vector<uint8_t>& out_aes_iv) {
    if (auth_key.size() != 256 || msg_key.size() != 16) {
        return false;
    }

    size_t x = is_client ? 0 : 8;

    // sha256_a = SHA256(msg_key + auth_key[x : x + 36])
    std::vector<uint8_t> data_a;
    data_a.reserve(16 + 36);
    data_a.insert(data_a.end(), msg_key.begin(), msg_key.end());
    data_a.insert(data_a.end(), auth_key.begin() + x, auth_key.begin() + x + 36);
    auto sha256_a = compute_sha256(data_a);

    // sha256_b = SHA256(auth_key[x + 40 : x + 76] + msg_key)
    std::vector<uint8_t> data_b;
    data_b.reserve(36 + 16);
    data_b.insert(data_b.end(), auth_key.begin() + x + 40, auth_key.begin() + x + 76);
    data_b.insert(data_b.end(), msg_key.begin(), msg_key.end());
    auto sha256_b = compute_sha256(data_b);

    // aes_key = sha256_a[0 : 8] + sha256_b[8 : 24] + sha256_a[24 : 32]
    out_aes_key.clear();
    out_aes_key.reserve(32);
    out_aes_key.insert(out_aes_key.end(), sha256_a.begin(), sha256_a.begin() + 8);
    out_aes_key.insert(out_aes_key.end(), sha256_b.begin() + 8, sha256_b.begin() + 24);
    out_aes_key.insert(out_aes_key.end(), sha256_a.begin() + 24, sha256_a.begin() + 32);

    // aes_iv  = sha256_b[0 : 8] + sha256_a[8 : 24] + sha256_b[24 : 32]
    out_aes_iv.clear();
    out_aes_iv.reserve(32);
    out_aes_iv.insert(out_aes_iv.end(), sha256_b.begin(), sha256_b.begin() + 8);
    out_aes_iv.insert(out_aes_iv.end(), sha256_a.begin() + 8, sha256_a.begin() + 24);
    out_aes_iv.insert(out_aes_iv.end(), sha256_b.begin() + 24, sha256_b.begin() + 32);

    return true;
}

std::vector<uint8_t> CryptoUtils::compute_msg_key(
    const std::vector<uint8_t>& auth_key,
    const std::vector<uint8_t>& plaintext,
    bool is_client) {
    if (auth_key.size() != 256) {
        throw std::invalid_argument("AuthKey must be 256 bytes");
    }

    size_t offset = is_client ? 88 : 96;
    std::vector<uint8_t> data;
    data.reserve(32 + plaintext.size());
    data.insert(data.end(), auth_key.begin() + offset, auth_key.begin() + offset + 32);
    data.insert(data.end(), plaintext.begin(), plaintext.end());

    auto hash = compute_sha256(data);
    // Middle 16 bytes: bytes 8 to 24
    return std::vector<uint8_t>(hash.begin() + 8, hash.begin() + 24);
}

std::vector<uint8_t> CryptoUtils::generate_random_bytes(size_t count) {
    std::vector<uint8_t> buf(count);
    if (RAND_bytes(buf.data(), static_cast<int>(count)) != 1) {
        for (size_t i = 0; i < count; ++i) {
            buf[i] = static_cast<uint8_t>(rand() & 0xFF);
        }
    }
    return buf;
}

uint64_t CryptoUtils::compute_auth_key_id(const std::vector<uint8_t>& auth_key) {
    if (auth_key.size() != 256) {
        return 0;
    }
    auto sha1_hash = compute_sha1(auth_key);
    // Lower 64 bits (8 bytes) of SHA1 hash (bytes 12..19 in little-endian order)
    uint64_t key_id = 0;
    for (size_t i = 0; i < 8; ++i) {
        key_id |= (static_cast<uint64_t>(sha1_hash[12 + i]) << (i * 8));
    }
    return key_id;
}

} // namespace cppgram
