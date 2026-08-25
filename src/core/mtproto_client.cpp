#include "cppgram/mtproto_client.hpp"

namespace cppgram {

MtprotoClient::MtprotoClient()
    : codec_(create_transport_codec(TransportProtocol::Intermediate)) {}

MtprotoClient::MtprotoClient(ClientConfig config)
    : config_(std::move(config)),
      codec_(create_transport_codec(TransportProtocol::Intermediate)),
      active_dc_id_(config_.primary_dc) {}

MtprotoClient::~MtprotoClient() {
    disconnect();
}

MtprotoClient::MtprotoClient(MtprotoClient&&) noexcept = default;
MtprotoClient& MtprotoClient::operator=(MtprotoClient&&) noexcept = default;

bool MtprotoClient::connect(int dc_id, TransportProtocol proto, int timeout_ms) {
    disconnect();

    const DataCenter* dc = dc_mgr_.get_dc(dc_id, config_.test_mode);
    if (!dc) {
        return false;
    }

    codec_ = create_transport_codec(proto);
    if (!codec_) {
        return false;
    }

    active_dc_id_ = dc_id;
    active_protocol_ = proto;

    if (!connection_.connect(dc->ip_v4, dc->port, timeout_ms)) {
        return false;
    }

    auto header = codec_->get_header();
    if (!header.empty()) {
        if (!connection_.send_bytes(header)) {
            connection_.disconnect();
            return false;
        }
    }

    return true;
}

void MtprotoClient::disconnect() {
    connection_.disconnect();
    if (codec_) {
        codec_->reset();
    }
}

bool MtprotoClient::is_connected() const noexcept {
    return connection_.is_connected();
}

bool MtprotoClient::send_unencrypted(const std::vector<uint8_t>& payload) {
    if (!is_connected() || !codec_) {
        return false;
    }
    auto packet = session_.pack_unencrypted_message(payload);
    return connection_.send_packet(*codec_, packet);
}

bool MtprotoClient::send_encrypted(const std::vector<uint8_t>& payload, bool is_content_related) {
    if (!is_connected() || !codec_) {
        return false;
    }
    auto packet = session_.pack_encrypted_message(payload, is_content_related);
    return connection_.send_packet(*codec_, packet);
}

std::vector<uint8_t> MtprotoClient::receive_response(int timeout_ms) {
    if (!is_connected() || !codec_) {
        return {};
    }
    return connection_.receive_packet(*codec_, timeout_ms);
}

std::vector<uint8_t> MtprotoClient::build_ping_query(int64_t ping_id) {
    TLWriter writer;
    writer.write_uint32(TL_PING);
    writer.write_int64(ping_id);
    return writer.take_data();
}

bool MtprotoClient::parse_pong_response(std::span<const uint8_t> data, int64_t& out_ping_id) {
    if (data.size() < 20) {
        return false;
    }
    try {
        TLReader reader(data);
        uint32_t ctor = reader.read_uint32();
        if (ctor != TL_PONG) {
            return false;
        }
        [[maybe_unused]] int64_t msg_id = reader.read_int64();
        out_ping_id = reader.read_int64();
        return true;
    } catch (...) {
        return false;
    }
}

bool MtprotoClient::ping(int64_t ping_id) {
    if (ping_id == 0) {
        ping_id = session_.generate_msg_id();
    }
    auto query = build_ping_query(ping_id);
    return send_encrypted(query, false);
}

} // namespace cppgram
