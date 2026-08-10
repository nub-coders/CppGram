#pragma once

/**
 * @file storage.hpp
 * @brief Persistent and in-memory session and peer storage interfaces for CppGram.
 */

#include <string>
#include <optional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "cppgram/types.hpp"

namespace cppgram {

/**
 * @brief Represents cached Telegram peer metadata (user, group, channel).
 */
struct PeerInfo {
    int64_t     id{0};
    int         type{0}; ///< 0 = Unknown, 1 = User, 2 = Group, 3 = Channel
    std::string access_hash;
    std::string username;
    std::string phone;
    std::string title;
};

/**
 * @brief Abstract interface for key-value session parameters and peer metadata caching.
 */
class ISessionStorage {
public:
    virtual ~ISessionStorage() = default;

    /**
     * @brief Persists an arbitrary string key-value pair.
     */
    virtual void set_value(const std::string& key, const std::string& value) = 0;

    /**
     * @brief Retrieves a string value by key if it exists.
     */
    virtual std::optional<std::string> get_value(const std::string& key) = 0;

    /**
     * @brief Deletes a key-value pair by key.
     */
    virtual void delete_value(const std::string& key) = 0;

    /**
     * @brief Persists or updates peer metadata.
     */
    virtual void save_peer(const PeerInfo& peer) = 0;

    /**
     * @brief Looks up cached peer metadata by numerical peer ID.
     */
    virtual std::optional<PeerInfo> get_peer(int64_t id) = 0;

    /**
     * @brief Looks up cached peer metadata by Telegram username.
     */
    virtual std::optional<PeerInfo> get_peer_by_username(const std::string& username) = 0;

    /**
     * @brief Closes storage resources and flushes pending transactions.
     */
    virtual void close() = 0;
};

/**
 * @brief Thread-safe in-memory session storage provider.
 */
class MemorySessionStorage : public ISessionStorage {
public:
    MemorySessionStorage() = default;
    ~MemorySessionStorage() override = default;

    void set_value(const std::string& key, const std::string& value) override {
        std::lock_guard<std::mutex> lock(mtx_);
        kv_[key] = value;
    }

    std::optional<std::string> get_value(const std::string& key) override {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = kv_.find(key);
        if (it != kv_.end()) return it->second;
        return std::nullopt;
    }

    void delete_value(const std::string& key) override {
        std::lock_guard<std::mutex> lock(mtx_);
        kv_.erase(key);
    }

    void save_peer(const PeerInfo& peer) override {
        std::lock_guard<std::mutex> lock(mtx_);
        peers_[peer.id] = peer;
        if (!peer.username.empty()) {
            username_to_id_[peer.username] = peer.id;
        }
    }

    std::optional<PeerInfo> get_peer(int64_t id) override {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = peers_.find(id);
        if (it != peers_.end()) return it->second;
        return std::nullopt;
    }

    std::optional<PeerInfo> get_peer_by_username(const std::string& username) override {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = username_to_id_.find(username);
        if (it != username_to_id_.end()) {
            auto pit = peers_.find(it->second);
            if (pit != peers_.end()) return pit->second;
        }
        return std::nullopt;
    }

    void close() override {
        std::lock_guard<std::mutex> lock(mtx_);
        kv_.clear();
        peers_.clear();
        username_to_id_.clear();
    }

private:
    std::mutex mtx_;
    std::unordered_map<std::string, std::string> kv_;
    std::unordered_map<int64_t, PeerInfo> peers_;
    std::unordered_map<std::string, int64_t> username_to_id_;
};

#ifdef CPPGRAM_HAVE_SQLITE3
/**
 * @brief Thread-safe SQLite3 persistent storage with WAL mode support.
 */
class SqliteSessionStorage : public ISessionStorage {
public:
    explicit SqliteSessionStorage(const std::string& db_path);
    ~SqliteSessionStorage() override;

    void set_value(const std::string& key, const std::string& value) override;
    std::optional<std::string> get_value(const std::string& key) override;
    void delete_value(const std::string& key) override;

    void save_peer(const PeerInfo& peer) override;
    std::optional<PeerInfo> get_peer(int64_t id) override;
    std::optional<PeerInfo> get_peer_by_username(const std::string& username) override;

    void close() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#endif

/**
 * @brief Factory function to create an ISessionStorage instance.
 * @param path Storage file path (e.g. ":memory:" or a filepath ending in .db/.session).
 * @return Shared pointer to the configured ISessionStorage implementation.
 */
std::shared_ptr<ISessionStorage> create_storage(const std::string& path = ":memory:");

} // namespace cppgram
