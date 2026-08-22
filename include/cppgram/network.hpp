#pragma once

#include "cppgram/transport.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace cppgram {

struct DataCenter {
    int id{1};
    std::string ip_v4;
    std::string ip_v6;
    int port{443};
    bool is_test{false};
    std::string name;
};

class DatacenterManager {
public:
    DatacenterManager();

    [[nodiscard]] const DataCenter* get_dc(int id, bool test = false) const;
    [[nodiscard]] std::vector<DataCenter> get_all_dcs(bool test = false) const;

    void set_primary_dc(int id);
    [[nodiscard]] int get_primary_dc_id() const noexcept { return primary_dc_id_; }
    [[nodiscard]] const DataCenter* get_primary_dc(bool test = false) const;

    void register_custom_dc(const DataCenter& dc);

private:
    void init_default_dcs();

    std::map<int, DataCenter> prod_dcs_;
    std::map<int, DataCenter> test_dcs_;
    int primary_dc_id_{2};
};

class TcpConnection {
public:
    TcpConnection();
    ~TcpConnection();

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    TcpConnection(TcpConnection&& other) noexcept;
    TcpConnection& operator=(TcpConnection&& other) noexcept;

    bool connect(const std::string& host, int port, int timeout_ms = 5000);
    void disconnect();
    [[nodiscard]] bool is_connected() const noexcept { return socket_fd_ >= 0; }

    bool send_bytes(const uint8_t* data, size_t size);
    bool send_bytes(const std::vector<uint8_t>& data);

    std::vector<uint8_t> receive_bytes(size_t max_bytes, int timeout_ms = 1000);

    bool send_packet(ITransportCodec& codec, const std::vector<uint8_t>& payload);
    std::vector<uint8_t> receive_packet(ITransportCodec& codec, int timeout_ms = 1000);

    [[nodiscard]] int native_handle() const noexcept { return socket_fd_; }

private:
    int socket_fd_{-1};
    std::vector<uint8_t> rx_buffer_;
};

} // namespace cppgram
