#pragma once

#include "cppgram/network.hpp"
#include "cppgram/session.hpp"
#include "cppgram/tl.hpp"
#include "cppgram/types.hpp"
#include <memory>
#include <vector>
#include <cstdint>
#include <span>

namespace cppgram {

class MtprotoClient {
public:
    MtprotoClient();
    explicit MtprotoClient(ClientConfig config);
    ~MtprotoClient();

    MtprotoClient(const MtprotoClient&) = delete;
    MtprotoClient& operator=(const MtprotoClient&) = delete;
    MtprotoClient(MtprotoClient&&) noexcept;
    MtprotoClient& operator=(MtprotoClient&&) noexcept;

    bool connect(int dc_id = 2, TransportProtocol proto = TransportProtocol::Intermediate, int timeout_ms = 5000);
    void disconnect();
    [[nodiscard]] bool is_connected() const noexcept;

    [[nodiscard]] int get_active_dc_id() const noexcept { return active_dc_id_; }
    [[nodiscard]] TransportProtocol get_active_protocol() const noexcept { return active_protocol_; }

    [[nodiscard]] Session& get_session() noexcept { return session_; }
    [[nodiscard]] const Session& get_session() const noexcept { return session_; }

    [[nodiscard]] DatacenterManager& get_dc_manager() noexcept { return dc_mgr_; }
    [[nodiscard]] const DatacenterManager& get_dc_manager() const noexcept { return dc_mgr_; }

    [[nodiscard]] ClientConfig& get_config() noexcept { return config_; }
    [[nodiscard]] const ClientConfig& get_config() const noexcept { return config_; }

    bool send_unencrypted(const std::vector<uint8_t>& payload);
    bool send_encrypted(const std::vector<uint8_t>& payload, bool is_content_related = true);
    bool send_with_layer(int32_t layer, const std::vector<uint8_t>& rpc_query, bool is_content_related = true);
    std::vector<uint8_t> receive_response(int timeout_ms = 3000);

    [[nodiscard]] int32_t get_layer() const noexcept { return config_.layer; }

    bool ping(int64_t ping_id = 0);
    static std::vector<uint8_t> build_ping_query(int64_t ping_id);
    static bool parse_pong_response(std::span<const uint8_t> data, int64_t& out_ping_id);
    static std::vector<uint8_t> build_invoke_with_layer_query(int32_t layer, const std::vector<uint8_t>& rpc_query);

private:
    ClientConfig config_;
    DatacenterManager dc_mgr_;
    std::unique_ptr<ITransportCodec> codec_;
    TcpConnection connection_;
    Session session_;
    int active_dc_id_{2};
    TransportProtocol active_protocol_{TransportProtocol::Intermediate};
};

} // namespace cppgram
