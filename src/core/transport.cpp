#include "cppgram/transport.hpp"
#include <cstring>
#include <array>

namespace cppgram {

// ---------------------------------------------------------------------------
// CRC32 Lookup Table
// ---------------------------------------------------------------------------
namespace {
class Crc32Table {
public:
    constexpr Crc32Table() : table_{} {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                if (c & 1) {
                    c = 0xEDB88320u ^ (c >> 1);
                } else {
                    c >>= 1;
                }
            }
            table_[i] = c;
        }
    }

    constexpr uint32_t operator[](size_t idx) const noexcept {
        return table_[idx];
    }

private:
    std::array<uint32_t, 256> table_;
};

constexpr Crc32Table kCrc32Table;
} // anonymous namespace

uint32_t FullCodec::compute_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        uint8_t index = static_cast<uint8_t>((crc ^ data[i]) & 0xFF);
        crc = (crc >> 8) ^ kCrc32Table[index];
    }
    return crc ^ 0xFFFFFFFFu;
}

// ---------------------------------------------------------------------------
// AbridgedCodec Implementation
// ---------------------------------------------------------------------------
std::vector<uint8_t> AbridgedCodec::get_header() const {
    return {0xef};
}

std::vector<uint8_t> AbridgedCodec::encode_packet(const uint8_t* data, size_t length) {
    size_t words = (length + 3) / 4;
    std::vector<uint8_t> out;

    if (words < 127) {
        out.reserve(1 + length);
        out.push_back(static_cast<uint8_t>(words));
    } else {
        out.reserve(4 + length);
        out.push_back(0x7f);
        out.push_back(static_cast<uint8_t>(words & 0xff));
        out.push_back(static_cast<uint8_t>((words >> 8) & 0xff));
        out.push_back(static_cast<uint8_t>((words >> 16) & 0xff));
    }

    out.insert(out.end(), data, data + length);
    return out;
}

std::vector<std::vector<uint8_t>> AbridgedCodec::decode_packets(const uint8_t* data, size_t length) {
    buffer_.insert(buffer_.end(), data, data + length);
    std::vector<std::vector<uint8_t>> packets;

    size_t offset = 0;
    while (offset < buffer_.size()) {
        uint8_t first = buffer_[offset];
        size_t header_len = 0;
        size_t payload_len = 0;

        if (first < 0x7f) {
            header_len = 1;
            payload_len = static_cast<size_t>(first) * 4;
        } else {
            if (buffer_.size() - offset < 4) break;
            header_len = 4;
            uint32_t words = static_cast<uint32_t>(buffer_[offset + 1]) |
                             (static_cast<uint32_t>(buffer_[offset + 2]) << 8) |
                             (static_cast<uint32_t>(buffer_[offset + 3]) << 16);
            payload_len = static_cast<size_t>(words) * 4;
        }

        if (buffer_.size() - offset < header_len + payload_len) {
            break;
        }

        const uint8_t* payload_start = buffer_.data() + offset + header_len;
        packets.emplace_back(payload_start, payload_start + payload_len);
        offset += header_len + payload_len;
    }

    if (offset > 0) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + offset);
    }

    return packets;
}

void AbridgedCodec::reset() {
    buffer_.clear();
}

// ---------------------------------------------------------------------------
// IntermediateCodec Implementation
// ---------------------------------------------------------------------------
std::vector<uint8_t> IntermediateCodec::get_header() const {
    return {0xee, 0xee, 0xee, 0xee};
}

std::vector<uint8_t> IntermediateCodec::encode_packet(const uint8_t* data, size_t length) {
    std::vector<uint8_t> out;
    out.reserve(4 + length);

    uint32_t len32 = static_cast<uint32_t>(length);
    out.push_back(static_cast<uint8_t>(len32 & 0xff));
    out.push_back(static_cast<uint8_t>((len32 >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>((len32 >> 16) & 0xff));
    out.push_back(static_cast<uint8_t>((len32 >> 24) & 0xff));

    out.insert(out.end(), data, data + length);
    return out;
}

std::vector<std::vector<uint8_t>> IntermediateCodec::decode_packets(const uint8_t* data, size_t length) {
    buffer_.insert(buffer_.end(), data, data + length);
    std::vector<std::vector<uint8_t>> packets;

    size_t offset = 0;
    while (offset + 4 <= buffer_.size()) {
        uint32_t payload_len = static_cast<uint32_t>(buffer_[offset]) |
                               (static_cast<uint32_t>(buffer_[offset + 1]) << 8) |
                               (static_cast<uint32_t>(buffer_[offset + 2]) << 16) |
                               (static_cast<uint32_t>(buffer_[offset + 3]) << 24);

        if (buffer_.size() - offset < 4 + payload_len) {
            break;
        }

        const uint8_t* payload_start = buffer_.data() + offset + 4;
        packets.emplace_back(payload_start, payload_start + payload_len);
        offset += 4 + payload_len;
    }

    if (offset > 0) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + offset);
    }

    return packets;
}

void IntermediateCodec::reset() {
    buffer_.clear();
}

// ---------------------------------------------------------------------------
// FullCodec Implementation
// ---------------------------------------------------------------------------
FullCodec::FullCodec() : out_seq_no_(0), in_seq_no_(0) {}

std::vector<uint8_t> FullCodec::get_header() const {
    return {};
}

std::vector<uint8_t> FullCodec::encode_packet(const uint8_t* data, size_t length) {
    uint32_t total_len = static_cast<uint32_t>(length + 12);
    std::vector<uint8_t> out;
    out.reserve(total_len);

    // 1. Length (4 bytes LE)
    out.push_back(static_cast<uint8_t>(total_len & 0xff));
    out.push_back(static_cast<uint8_t>((total_len >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>((total_len >> 16) & 0xff));
    out.push_back(static_cast<uint8_t>((total_len >> 24) & 0xff));

    // 2. Sequence Number (4 bytes LE)
    uint32_t seq = out_seq_no_++;
    out.push_back(static_cast<uint8_t>(seq & 0xff));
    out.push_back(static_cast<uint8_t>((seq >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>((seq >> 16) & 0xff));
    out.push_back(static_cast<uint8_t>((seq >> 24) & 0xff));

    // 3. Payload
    out.insert(out.end(), data, data + length);

    // 4. CRC32 Checksum over length + seq + payload
    uint32_t crc = compute_crc32(out.data(), out.size());
    out.push_back(static_cast<uint8_t>(crc & 0xff));
    out.push_back(static_cast<uint8_t>((crc >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>((crc >> 16) & 0xff));
    out.push_back(static_cast<uint8_t>((crc >> 24) & 0xff));

    return out;
}

std::vector<std::vector<uint8_t>> FullCodec::decode_packets(const uint8_t* data, size_t length) {
    buffer_.insert(buffer_.end(), data, data + length);
    std::vector<std::vector<uint8_t>> packets;

    size_t offset = 0;
    while (offset + 12 <= buffer_.size()) {
        uint32_t total_len = static_cast<uint32_t>(buffer_[offset]) |
                             (static_cast<uint32_t>(buffer_[offset + 1]) << 8) |
                             (static_cast<uint32_t>(buffer_[offset + 2]) << 16) |
                             (static_cast<uint32_t>(buffer_[offset + 3]) << 24);

        if (total_len < 12 || total_len > 16 * 1024 * 1024) {
            throw std::runtime_error("FullCodec invalid packet length: " + std::to_string(total_len));
        }

        if (buffer_.size() - offset < total_len) {
            break;
        }

        // Validate CRC32
        uint32_t expected_crc = static_cast<uint32_t>(buffer_[offset + total_len - 4]) |
                                (static_cast<uint32_t>(buffer_[offset + total_len - 3]) << 8) |
                                (static_cast<uint32_t>(buffer_[offset + total_len - 2]) << 16) |
                                (static_cast<uint32_t>(buffer_[offset + total_len - 1]) << 24);

        uint32_t calculated_crc = compute_crc32(buffer_.data() + offset, total_len - 4);
        if (expected_crc != calculated_crc) {
            throw std::runtime_error("FullCodec CRC32 mismatch");
        }

        size_t payload_len = total_len - 12;
        const uint8_t* payload_start = buffer_.data() + offset + 8;
        packets.emplace_back(payload_start, payload_start + payload_len);

        ++in_seq_no_;
        offset += total_len;
    }

    if (offset > 0) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + offset);
    }

    return packets;
}

void FullCodec::reset() {
    buffer_.clear();
    out_seq_no_ = 0;
    in_seq_no_ = 0;
}

// ---------------------------------------------------------------------------
// Factory Function
// ---------------------------------------------------------------------------
std::unique_ptr<ITransportCodec> create_transport_codec(TransportProtocol protocol) {
    switch (protocol) {
        case TransportProtocol::Abridged:
            return std::make_unique<AbridgedCodec>();
        case TransportProtocol::Intermediate:
        case TransportProtocol::PaddedIntermediate:
            return std::make_unique<IntermediateCodec>();
        case TransportProtocol::Full:
            return std::make_unique<FullCodec>();
        default:
            return std::make_unique<AbridgedCodec>();
    }
}

} // namespace cppgram
