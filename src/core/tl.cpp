#include "cppgram/tl.hpp"
#include <cstring>

namespace cppgram {

void TLWriter::write_int32(int32_t val) {
    write_uint32(static_cast<uint32_t>(val));
}

void TLWriter::write_uint32(uint32_t val) {
    uint8_t bytes[4];
    std::memcpy(bytes, &val, 4);
    buffer_.insert(buffer_.end(), bytes, bytes + 4);
}

void TLWriter::write_int64(int64_t val) {
    write_uint64(static_cast<uint64_t>(val));
}

void TLWriter::write_uint64(uint64_t val) {
    uint8_t bytes[8];
    std::memcpy(bytes, &val, 8);
    buffer_.insert(buffer_.end(), bytes, bytes + 8);
}

void TLWriter::write_int128(const std::array<uint8_t, 16>& val) {
    buffer_.insert(buffer_.end(), val.begin(), val.end());
}

void TLWriter::write_int256(const std::array<uint8_t, 32>& val) {
    buffer_.insert(buffer_.end(), val.begin(), val.end());
}

void TLWriter::write_double(double val) {
    uint8_t bytes[8];
    std::memcpy(bytes, &val, 8);
    buffer_.insert(buffer_.end(), bytes, bytes + 8);
}

void TLWriter::write_string(const std::string& str) {
    write_bytes(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
}

void TLWriter::write_bytes(const std::vector<uint8_t>& bytes) {
    write_bytes(std::span<const uint8_t>(bytes.data(), bytes.size()));
}

void TLWriter::write_bytes(std::span<const uint8_t> bytes) {
    size_t len = bytes.size();
    if (len <= 253) {
        buffer_.push_back(static_cast<uint8_t>(len));
        buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        size_t pad = (4 - ((1 + len) % 4)) % 4;
        buffer_.insert(buffer_.end(), pad, 0);
    } else {
        buffer_.push_back(0xfe);
        buffer_.push_back(static_cast<uint8_t>(len & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
        buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        size_t pad = (4 - ((4 + len) % 4)) % 4;
        buffer_.insert(buffer_.end(), pad, 0);
    }
}

void TLWriter::write_vector_header(size_t count) {
    write_uint32(TL_VECTOR);
    write_uint32(static_cast<uint32_t>(count));
}

// ---------------------------------------------------------------------------
// TLReader
// ---------------------------------------------------------------------------

TLReader::TLReader(std::span<const uint8_t> data) : data_(data), offset_(0) {}

TLReader::TLReader(const std::vector<uint8_t>& data)
    : data_(data.data(), data.size()), offset_(0) {}

bool TLReader::has_remaining(size_t bytes) const noexcept {
    return offset_ + bytes <= data_.size();
}

size_t TLReader::remaining_bytes() const noexcept {
    return offset_ < data_.size() ? data_.size() - offset_ : 0;
}

void TLReader::set_offset(size_t offset) {
    if (offset > data_.size()) {
        throw std::out_of_range("TLReader offset out of range");
    }
    offset_ = offset;
}

int32_t TLReader::read_int32() {
    return static_cast<int32_t>(read_uint32());
}

uint32_t TLReader::read_uint32() {
    if (!has_remaining(4)) {
        throw std::underflow_error("TLReader buffer underflow while reading uint32");
    }
    uint32_t val = 0;
    std::memcpy(&val, data_.data() + offset_, 4);
    offset_ += 4;
    return val;
}

int64_t TLReader::read_int64() {
    return static_cast<int64_t>(read_uint64());
}

uint64_t TLReader::read_uint64() {
    if (!has_remaining(8)) {
        throw std::underflow_error("TLReader buffer underflow while reading uint64");
    }
    uint64_t val = 0;
    std::memcpy(&val, data_.data() + offset_, 8);
    offset_ += 8;
    return val;
}

std::array<uint8_t, 16> TLReader::read_int128() {
    if (!has_remaining(16)) {
        throw std::underflow_error("TLReader buffer underflow while reading int128");
    }
    std::array<uint8_t, 16> val;
    std::memcpy(val.data(), data_.data() + offset_, 16);
    offset_ += 16;
    return val;
}

std::array<uint8_t, 32> TLReader::read_int256() {
    if (!has_remaining(32)) {
        throw std::underflow_error("TLReader buffer underflow while reading int256");
    }
    std::array<uint8_t, 32> val;
    std::memcpy(val.data(), data_.data() + offset_, 32);
    offset_ += 32;
    return val;
}

double TLReader::read_double() {
    if (!has_remaining(8)) {
        throw std::underflow_error("TLReader buffer underflow while reading double");
    }
    double val = 0.0;
    std::memcpy(&val, data_.data() + offset_, 8);
    offset_ += 8;
    return val;
}

std::vector<uint8_t> TLReader::read_bytes() {
    if (!has_remaining(1)) {
        throw std::underflow_error("TLReader buffer underflow while reading string/bytes length");
    }

    uint8_t first_byte = data_[offset_++];
    size_t len = 0;
    size_t header_len = 1;

    if (first_byte <= 253) {
        len = first_byte;
    } else if (first_byte == 0xfe) {
        if (!has_remaining(3)) {
            throw std::underflow_error("TLReader buffer underflow while reading extended length");
        }
        uint32_t b0 = data_[offset_++];
        uint32_t b1 = data_[offset_++];
        uint32_t b2 = data_[offset_++];
        len = b0 | (b1 << 8) | (b2 << 16);
        header_len = 4;
    } else {
        throw std::runtime_error("Invalid TL length prefix byte");
    }

    if (!has_remaining(len)) {
        throw std::underflow_error("TLReader buffer underflow while reading string/bytes content");
    }

    std::vector<uint8_t> result(data_.data() + offset_, data_.data() + offset_ + len);
    offset_ += len;

    size_t pad = (4 - ((header_len + len) % 4)) % 4;
    if (!has_remaining(pad)) {
        throw std::underflow_error("TLReader buffer underflow while reading padding");
    }
    offset_ += pad;

    return result;
}

std::string TLReader::read_string() {
    auto bytes = read_bytes();
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

uint32_t TLReader::read_vector_header() {
    uint32_t ctor = read_uint32();
    if (ctor != TL_VECTOR) {
        throw std::runtime_error("Invalid vector constructor ID");
    }
    return read_uint32();
}

bool TLReader::read_invoke_with_layer(int32_t& out_layer, std::vector<uint8_t>& out_query) {
    if (!has_remaining(8)) {
        return false;
    }
    size_t save_offset = offset_;
    uint32_t ctor = read_uint32();
    if (ctor != TL_INVOKE_WITH_LAYER) {
        offset_ = save_offset;
        return false;
    }
    out_layer = read_int32();
    size_t rem = remaining_bytes();
    out_query.assign(data_.data() + offset_, data_.data() + offset_ + rem);
    offset_ += rem;
    return true;
}

void TLWriter::write_invoke_with_layer(int32_t layer, const std::vector<uint8_t>& query) {
    write_uint32(TL_INVOKE_WITH_LAYER);
    write_int32(layer);
    buffer_.insert(buffer_.end(), query.begin(), query.end());
}

void TLWriter::write_init_connection(
    int32_t api_id,
    const std::string& device_model,
    const std::string& system_version,
    const std::string& app_version,
    const std::string& system_lang_code,
    const std::string& lang_pack,
    const std::string& lang_code,
    const std::vector<uint8_t>& query) {
    write_uint32(TL_INIT_CONNECTION);
    write_uint32(0); // flags:#
    write_int32(api_id);
    write_string(device_model);
    write_string(system_version);
    write_string(app_version);
    write_string(system_lang_code);
    write_string(lang_pack);
    write_string(lang_code);
    buffer_.insert(buffer_.end(), query.begin(), query.end());
}

} // namespace cppgram
