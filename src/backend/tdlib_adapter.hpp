#pragma once
// Internal header — wraps td::ClientManager with async request/response plumbing.
// Only included by client.cpp.

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "cppgram/types.hpp"
#include "cppgram/log.hpp"

namespace cppgram::detail {

namespace td_api = td::td_api;
using Object  = td_api::object_ptr<td_api::Object>;
using Handler = std::function<void(Object)>;

/// Thin async wrapper around td::ClientManager.
class TdLibAdapter {
public:
    TdLibAdapter() {
        td::ClientManager::execute(
            td_api::make_object<td_api::setLogVerbosityLevel>(1));
        mgr_       = std::make_unique<td::ClientManager>();
        client_id_ = mgr_->create_client_id();
        // Kick TDLib into life with a dummy request.
        send(td_api::make_object<td_api::getOption>("version"), {});
    }

    ~TdLibAdapter() { stop(); }

    TdLibAdapter(const TdLibAdapter&) = delete;
    TdLibAdapter& operator=(const TdLibAdapter&) = delete;

    /// Send a TDLib function and register a one-shot callback for its result.
    std::uint64_t send(td_api::object_ptr<td_api::Function> fn, Handler handler) {
        auto id = next_id_.fetch_add(1);
        if (handler) {
            std::lock_guard lk(handlers_mtx_);
            handlers_[id] = std::move(handler);
        }
        mgr_->send(client_id_, id, std::move(fn));
        return id;
    }

    /// Synchronous send — blocks until the response arrives.
    Object send_sync(td_api::object_ptr<td_api::Function> fn) {
        std::promise<Object> promise;
        auto future = promise.get_future();
        send(std::move(fn), [&promise](Object obj) {
            promise.set_value(std::move(obj));
        });
        return future.get();
    }

    /// Set the callback for incoming updates (request_id == 0).
    void set_update_handler(Handler h) {
        std::lock_guard lk(handlers_mtx_);
        update_handler_ = std::move(h);
    }

    /// Start the receive thread.
    void start() {
        if (running_) return;
        running_ = true;
        recv_thread_ = std::thread([this] { recv_loop(); });
    }

    /// Stop the receive thread.
    void stop() {
        running_ = false;
        if (recv_thread_.joinable()) recv_thread_.join();
    }

    bool running() const noexcept { return running_; }

private:
    void recv_loop() {
        while (running_) {
            auto response = mgr_->receive(1.0);
            if (!response.object) continue;

            if (response.request_id == 0) {
                // Incoming update.
                Handler h;
                {
                    std::lock_guard lk(handlers_mtx_);
                    h = update_handler_;
                }
                if (h) h(std::move(response.object));
            } else {
                // Response to a specific request.
                Handler h;
                {
                    std::lock_guard lk(handlers_mtx_);
                    auto it = handlers_.find(response.request_id);
                    if (it != handlers_.end()) {
                        h = std::move(it->second);
                        handlers_.erase(it);
                    }
                }
                if (h) h(std::move(response.object));
            }
        }
    }

    std::unique_ptr<td::ClientManager> mgr_;
    std::int32_t client_id_{};
    std::thread  recv_thread_;
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> next_id_{1};

    std::mutex handlers_mtx_;
    std::unordered_map<std::uint64_t, Handler> handlers_;
    Handler update_handler_;
};

} // namespace cppgram::detail
