#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <array>
#include <span>
#include <stdexcept>

namespace cppgram {

// Telegram Protocol API Layer
constexpr int32_t TELEGRAM_API_LAYER = 225;

// Standard Telegram TL Constructor IDs
constexpr uint32_t TL_VECTOR          = 0x1cb5c415;
constexpr uint32_t TL_REQ_PQ_MULTI    = 0xbe7e8ef1;
constexpr uint32_t TL_RES_PQ          = 0x05162463;
constexpr uint32_t TL_REQ_DH_PARAMS   = 0xd712e4be;
constexpr uint32_t TL_SERVER_DH_OK    = 0xd0e8075c;
constexpr uint32_t TL_CLIENT_DH_INNER = 0x6643b654;
constexpr uint32_t TL_DH_GEN_OK       = 0x3bcbf734;
constexpr uint32_t TL_PING            = 0x7abe77ec;
constexpr uint32_t TL_PONG            = 0x347773c5;
constexpr uint32_t TL_PING_DELAY_DISC = 0xf3427456;
constexpr uint32_t TL_RPC_RESULT      = 0xf35c6d01;
constexpr uint32_t TL_RPC_ERROR       = 0x2144ca19;
constexpr uint32_t TL_GZIP_PACKED     = 0x3072c41e;

// Layer 225 Envelope & Method Constructor IDs
constexpr uint32_t TL_INVOKE_WITH_LAYER                  = 0xda9b0d0d;
constexpr uint32_t TL_INIT_CONNECTION                    = 0xc1cd5ea9;
constexpr uint32_t TL_MESSAGES_SET_BOT_GUEST_CHAT_RESULT = 0x052b08db;
constexpr uint32_t TL_MESSAGES_DELETE_PARTICIPANT_REACTIONS = 0xa0b80cf8;
constexpr uint32_t TL_MESSAGES_DELETE_PARTICIPANT_REACTION  = 0xe3b7f82c;
constexpr uint32_t TL_STATS_GET_POLL_STATS               = 0xc27dfa68;
constexpr uint32_t TL_AICOMPOSE_CREATE_TONE              = 0x4aa83913;
constexpr uint32_t TL_AICOMPOSE_UPDATE_TONE              = 0x903bcf59;
constexpr uint32_t TL_AICOMPOSE_SAVE_TONE                = 0x1782cbb1;
constexpr uint32_t TL_AICOMPOSE_DELETE_TONE              = 0xdd39316a;
constexpr uint32_t TL_AICOMPOSE_GET_TONE                 = 0xb2e8ba03;
constexpr uint32_t TL_AICOMPOSE_GET_TONES                = 0xabd59201;
constexpr uint32_t TL_AICOMPOSE_GET_TONE_EXAMPLE         = 0xd1b4ab14;
constexpr uint32_t TL_AICOMPOSE_TONES                    = 0x6c9d0efe;
constexpr uint32_t TL_AICOMPOSE_TONES_NOT_MODIFIED       = 0xc1f46103;

class TLWriter {
public:
    TLWriter() = default;

    void write_int32(int32_t val);
    void write_uint32(uint32_t val);
    void write_int64(int64_t val);
    void write_uint64(uint64_t val);
    void write_int128(const std::array<uint8_t, 16>& val);
    void write_int256(const std::array<uint8_t, 32>& val);
    void write_double(double val);
    void write_string(const std::string& str);
    void write_bytes(const std::vector<uint8_t>& bytes);
    void write_bytes(std::span<const uint8_t> bytes);
    void write_vector_header(size_t count);
    void write_invoke_with_layer(int32_t layer, const std::vector<uint8_t>& query);
    void write_init_connection(
        int32_t api_id,
        const std::string& device_model,
        const std::string& system_version,
        const std::string& app_version,
        const std::string& system_lang_code,
        const std::string& lang_pack,
        const std::string& lang_code,
        const std::vector<uint8_t>& query);

    [[nodiscard]] const std::vector<uint8_t>& data() const noexcept { return buffer_; }
    [[nodiscard]] std::vector<uint8_t> take_data() noexcept { return std::move(buffer_); }
    [[nodiscard]] size_t size() const noexcept { return buffer_.size(); }
    void clear() noexcept { buffer_.clear(); }

private:
    std::vector<uint8_t> buffer_;
};

class TLReader {
public:
    explicit TLReader(std::span<const uint8_t> data);
    explicit TLReader(const std::vector<uint8_t>& data);

    int32_t read_int32();
    uint32_t read_uint32();
    int64_t read_int64();
    uint64_t read_uint64();
    std::array<uint8_t, 16> read_int128();
    std::array<uint8_t, 32> read_int256();
    double read_double();
    std::string read_string();
    std::vector<uint8_t> read_bytes();
    uint32_t read_vector_header();
    bool read_invoke_with_layer(int32_t& out_layer, std::vector<uint8_t>& out_query);

    [[nodiscard]] bool has_remaining(size_t bytes = 1) const noexcept;
    [[nodiscard]] size_t remaining_bytes() const noexcept;
    [[nodiscard]] size_t get_offset() const noexcept { return offset_; }
    void set_offset(size_t offset);

private:
    std::span<const uint8_t> data_;
    size_t offset_{0};
};

} // namespace cppgram
