#pragma once

/**
 * @file transport.hpp
 * @brief MTProto TCP transport packet framing and serialization codecs.
 */

#include <vector>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <stdexcept>

namespace cppgram {

/**
 * @brief Supported MTProto transport packet protocols.
 */
enum class TransportProtocol {
    Abridged,
    Intermediate,
    PaddedIntermediate,
    Full
};

/**
 * @brief Abstract interface for MTProto TCP transport stream packet framing codecs.
 */
class ITransportCodec {
public:
    virtual ~ITransportCodec() = default;

    /**
     * @brief Returns the handshake protocol header byte sequence to be sent upon connection.
     */
    virtual std::vector<uint8_t> get_header() const = 0;

    /**
     * @brief Encapsulates a raw MTProto payload into a framed packet according to the protocol.
     */
    virtual std::vector<uint8_t> encode_packet(const uint8_t* data, size_t length) = 0;
    std::vector<uint8_t> encode_packet(const std::vector<uint8_t>& data) {
        return encode_packet(data.data(), data.size());
    }

    /**
     * @brief Feeds incoming stream chunk and extracts all fully received raw MTProto payloads.
     */
    virtual std::vector<std::vector<uint8_t>> decode_packets(const uint8_t* data, size_t length) = 0;
    std::vector<std::vector<uint8_t>> decode_packets(const std::vector<uint8_t>& data) {
        return decode_packets(data.data(), data.size());
    }

    /**
     * @brief Clears any partial packet buffers and resets sequence counters.
     */
    virtual void reset() = 0;
};

/**
 * @brief Abridged transport codec (0xef prefix, compact length byte encoding).
 */
class AbridgedCodec : public ITransportCodec {
public:
    AbridgedCodec() = default;

    std::vector<uint8_t> get_header() const override;
    std::vector<uint8_t> encode_packet(const uint8_t* data, size_t length) override;
    std::vector<std::vector<uint8_t>> decode_packets(const uint8_t* data, size_t length) override;
    void reset() override;

private:
    std::vector<uint8_t> buffer_;
};

/**
 * @brief Intermediate transport codec (0xeeeeeeee prefix, 4-byte length prefix).
 */
class IntermediateCodec : public ITransportCodec {
public:
    IntermediateCodec() = default;

    std::vector<uint8_t> get_header() const override;
    std::vector<uint8_t> encode_packet(const uint8_t* data, size_t length) override;
    std::vector<std::vector<uint8_t>> decode_packets(const uint8_t* data, size_t length) override;
    void reset() override;

private:
    std::vector<uint8_t> buffer_;
};

/**
 * @brief Full transport codec with sequence numbering and CRC32 verification.
 */
class FullCodec : public ITransportCodec {
public:
    FullCodec();

    std::vector<uint8_t> get_header() const override;
    std::vector<uint8_t> encode_packet(const uint8_t* data, size_t length) override;
    std::vector<std::vector<uint8_t>> decode_packets(const uint8_t* data, size_t length) override;
    void reset() override;

    uint32_t out_seq_no() const noexcept { return out_seq_no_; }
    uint32_t in_seq_no() const noexcept { return in_seq_no_; }

    static uint32_t compute_crc32(const uint8_t* data, size_t length);

private:
    uint32_t out_seq_no_{0};
    uint32_t in_seq_no_{0};
    std::vector<uint8_t> buffer_;
};

/**
 * @brief Factory helper creating a transport codec instance.
 */
std::unique_ptr<ITransportCodec> create_transport_codec(TransportProtocol protocol);

} // namespace cppgram
