#pragma once

/**
 * @file middleware.hpp
 * @brief Interceptor pipeline for pre- and post-processing incoming Telegram events.
 */

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include "cppgram/message.hpp"
#include "cppgram/callback_query.hpp"

namespace cppgram {

class Client;

/**
 * @brief Context passed through the middleware pipeline containing the event and custom metadata.
 */
class MiddlewareContext {
public:
    explicit MiddlewareContext(Client& client, std::optional<Message> msg = std::nullopt,
                               std::optional<CallbackQuery> cb = std::nullopt)
        : client_(client), message_(std::move(msg)), callback_query_(std::move(cb)), stopped_(false) {}

    /**
     * @brief Access the associated CppGram Client instance.
     */
    Client& client() noexcept { return client_; }

    /**
     * @brief Checks if the intercepted event is a Message update.
     */
    bool is_message() const noexcept { return message_.has_value(); }

    /**
     * @brief Checks if the intercepted event is a CallbackQuery update.
     */
    bool is_callback_query() const noexcept { return callback_query_.has_value(); }

    /**
     * @brief Returns a reference to the optional Message.
     */
    std::optional<Message>& message() noexcept { return message_; }
    const std::optional<Message>& message() const noexcept { return message_; }

    /**
     * @brief Returns a reference to the optional CallbackQuery.
     */
    std::optional<CallbackQuery>& callback_query() noexcept { return callback_query_; }
    const std::optional<CallbackQuery>& callback_query() const noexcept { return callback_query_; }

    /**
     * @brief Stores metadata accessible by subsequent middleware and handlers.
     */
    void set_data(const std::string& key, const std::string& value) {
        data_[key] = value;
    }

    /**
     * @brief Retrieves stored metadata by key.
     */
    std::optional<std::string> get_data(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) return it->second;
        return std::nullopt;
    }

    /**
     * @brief Signals that subsequent middleware and event handlers should NOT be executed.
     */
    void stop_propagation() noexcept {
        stopped_ = true;
    }

    /**
     * @brief Checks if propagation has been cancelled.
     */
    bool is_stopped() const noexcept {
        return stopped_;
    }

private:
    Client& client_;
    std::optional<Message> message_;
    std::optional<CallbackQuery> callback_query_;
    std::unordered_map<std::string, std::string> data_;
    bool stopped_;
};

/**
 * @brief Middleware function signature. Return false or call ctx.stop_propagation() to halt the pipeline.
 */
using MiddlewareFunc = std::function<bool(MiddlewareContext&)>;

/**
 * @brief Pipeline managing registered middleware interceptors.
 */
class MiddlewarePipeline {
public:
    MiddlewarePipeline() = default;

    /**
     * @brief Appends a middleware interceptor to the execution pipeline.
     */
    void use(MiddlewareFunc mw) {
        std::lock_guard<std::mutex> lock(mtx_);
        middlewares_.push_back(std::move(mw));
    }

    /**
     * @brief Clears all registered middleware interceptors.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        middlewares_.clear();
    }

    /**
     * @brief Executes the pipeline against the given context.
     * @return True if the event should continue to handlers; false if aborted.
     */
    bool execute(MiddlewareContext& ctx) const {
        std::vector<MiddlewareFunc> copy;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            copy = middlewares_;
        }
        for (const auto& mw : copy) {
            if (ctx.is_stopped()) return false;
            try {
                bool proceed = mw(ctx);
                if (!proceed || ctx.is_stopped()) {
                    ctx.stop_propagation();
                    return false;
                }
            } catch (...) {
                ctx.stop_propagation();
                return false;
            }
        }
        return !ctx.is_stopped();
    }

    /**
     * @brief Returns the total number of registered middleware functions.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return middlewares_.size();
    }

    /**
     * @brief Checks if the pipeline has no registered middleware functions.
     */
    bool empty() const {
        return size() == 0;
    }

private:
    mutable std::mutex mtx_;
    std::vector<MiddlewareFunc> middlewares_;
};

} // namespace cppgram
