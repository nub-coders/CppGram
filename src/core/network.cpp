#include "cppgram/network.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <iostream>

namespace cppgram {

DatacenterManager::DatacenterManager() {
    init_default_dcs();
}

void DatacenterManager::init_default_dcs() {
    // Standard Telegram Production DCs
    prod_dcs_[1] = {1, "149.154.175.53", "", 443, false, "DC 1 (Miami, US)"};
    prod_dcs_[2] = {2, "149.154.167.51", "", 443, false, "DC 2 (Amsterdam, NL)"};
    prod_dcs_[3] = {3, "149.154.175.100", "", 443, false, "DC 3 (Miami, US)"};
    prod_dcs_[4] = {4, "149.154.167.91", "", 443, false, "DC 4 (Amsterdam, NL)"};
    prod_dcs_[5] = {5, "91.108.56.130", "", 443, false, "DC 5 (Singapore, SG)"};

    // Telegram Test DCs
    test_dcs_[1] = {1, "149.154.175.10", "", 443, true, "DC 1 Test (Miami, US)"};
    test_dcs_[2] = {2, "149.154.167.40", "", 443, true, "DC 2 Test (Amsterdam, NL)"};
    test_dcs_[3] = {3, "149.154.175.117", "", 443, true, "DC 3 Test (Miami, US)"};
}

const DataCenter* DatacenterManager::get_dc(int id, bool test) const {
    const auto& map = test ? test_dcs_ : prod_dcs_;
    auto it = map.find(id);
    if (it != map.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<DataCenter> DatacenterManager::get_all_dcs(bool test) const {
    const auto& map = test ? test_dcs_ : prod_dcs_;
    std::vector<DataCenter> res;
    res.reserve(map.size());
    for (const auto& [_, dc] : map) {
        res.push_back(dc);
    }
    return res;
}

void DatacenterManager::set_primary_dc(int id) {
    if (prod_dcs_.find(id) != prod_dcs_.end()) {
        primary_dc_id_ = id;
    }
}

const DataCenter* DatacenterManager::get_primary_dc(bool test) const {
    return get_dc(primary_dc_id_, test);
}

void DatacenterManager::register_custom_dc(const DataCenter& dc) {
    if (dc.is_test) {
        test_dcs_[dc.id] = dc;
    } else {
        prod_dcs_[dc.id] = dc;
    }
}

// ---------------------------------------------------------------------------
// TcpConnection
// ---------------------------------------------------------------------------

TcpConnection::TcpConnection() = default;

TcpConnection::~TcpConnection() {
    disconnect();
}

TcpConnection::TcpConnection(TcpConnection&& other) noexcept
    : socket_fd_(other.socket_fd_), rx_buffer_(std::move(other.rx_buffer_)) {
    other.socket_fd_ = -1;
}

TcpConnection& TcpConnection::operator=(TcpConnection&& other) noexcept {
    if (this != &other) {
        disconnect();
        socket_fd_ = other.socket_fd_;
        rx_buffer_ = std::move(other.rx_buffer_);
        other.socket_fd_ = -1;
    }
    return *this;
}

bool TcpConnection::connect(const std::string& host, int port, int timeout_ms) {
    disconnect();

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
        return false;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return false;
    }

    // Set non-blocking for connect timeout
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int rc = ::connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (rc < 0 && errno != EINPROGRESS) {
        close(fd);
        return false;
    }

    if (rc != 0) {
        struct pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLOUT;

        int poll_rc = poll(&pfd, 1, timeout_ms);
        if (poll_rc <= 0) {
            close(fd);
            return false;
        }

        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error != 0) {
            close(fd);
            return false;
        }
    }

    // Restore blocking flags
    fcntl(fd, F_SETFL, flags);
    socket_fd_ = fd;
    rx_buffer_.clear();
    return true;
}

void TcpConnection::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    rx_buffer_.clear();
}

bool TcpConnection::send_bytes(const uint8_t* data, size_t size) {
    if (socket_fd_ < 0 || !data || size == 0) {
        return false;
    }
    size_t total_sent = 0;
    while (total_sent < size) {
        ssize_t sent = ::send(socket_fd_, data + total_sent, size - total_sent, 0);
        if (sent <= 0) {
            return false;
        }
        total_sent += static_cast<size_t>(sent);
    }
    return true;
}

bool TcpConnection::send_bytes(const std::vector<uint8_t>& data) {
    return send_bytes(data.data(), data.size());
}

std::vector<uint8_t> TcpConnection::receive_bytes(size_t max_bytes, int timeout_ms) {
    if (socket_fd_ < 0 || max_bytes == 0) {
        return {};
    }

    struct pollfd pfd{};
    pfd.fd = socket_fd_;
    pfd.events = POLLIN;

    int poll_rc = poll(&pfd, 1, timeout_ms);
    if (poll_rc <= 0) {
        return {};
    }

    std::vector<uint8_t> buffer(max_bytes);
    ssize_t received = ::recv(socket_fd_, buffer.data(), max_bytes, 0);
    if (received <= 0) {
        return {};
    }
    buffer.resize(static_cast<size_t>(received));
    return buffer;
}

bool TcpConnection::send_packet(ITransportCodec& codec, const std::vector<uint8_t>& payload) {
    if (!is_connected()) {
        return false;
    }
    auto frame = codec.encode_packet(payload);
    return send_bytes(frame);
}

std::vector<uint8_t> TcpConnection::receive_packet(ITransportCodec& codec, int timeout_ms) {
    if (!is_connected()) {
        return {};
    }

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        if (!rx_buffer_.empty()) {
            auto packets = codec.decode_packets(rx_buffer_);
            if (!packets.empty()) {
                rx_buffer_.clear();
                return packets.front();
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        int remaining_ms = timeout_ms - static_cast<int>(elapsed);
        if (remaining_ms <= 0) {
            break;
        }

        auto chunk = receive_bytes(4096, remaining_ms);
        if (chunk.empty()) {
            break;
        }
        rx_buffer_.insert(rx_buffer_.end(), chunk.begin(), chunk.end());
    }

    return {};
}

} // namespace cppgram
