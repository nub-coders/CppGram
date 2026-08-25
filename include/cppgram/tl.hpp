#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <array>
#include <span>
#include <stdexcept>

namespace cppgram {

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

    [[nodiscard]] bool has_remaining(size_t bytes = 1) const noexcept;
    [[nodiscard]] size_t remaining_bytes() const noexcept;
    [[nodiscard]] size_t get_offset() const noexcept { return offset_; }
    void set_offset(size_t offset);

private:
    std::span<const uint8_t> data_;
    size_t offset_{0};
};

} // namespace cppgram
