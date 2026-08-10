#include "cppgram/storage.hpp"
#include "cppgram/errors.hpp"
#include "cppgram/log.hpp"
#include <iostream>

#ifdef CPPGRAM_HAVE_SQLITE3
#include <sqlite3.h>
#endif

namespace cppgram {

#ifdef CPPGRAM_HAVE_SQLITE3
struct SqliteSessionStorage::Impl {
    sqlite3* db{nullptr};
    std::mutex mtx;

    explicit Impl(const std::string& db_path) {
        int rc = sqlite3_open_v2(db_path.c_str(), &db,
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                                 nullptr);
        if (rc != SQLITE_OK) {
            std::string err_msg = db ? sqlite3_errmsg(db) : "Failed to allocate sqlite handle";
            if (db) {
                sqlite3_close(db);
                db = nullptr;
            }
            throw StorageError("Failed to open SQLite database at '" + db_path + "': " + err_msg);
        }

        const char* schema = R"(
            CREATE TABLE IF NOT EXISTS kv_store (
                key TEXT PRIMARY KEY,
                value TEXT,
                updated_at INTEGER
            );
            CREATE TABLE IF NOT EXISTS peers (
                id INTEGER PRIMARY KEY,
                type INTEGER,
                access_hash TEXT,
                username TEXT,
                phone TEXT,
                title TEXT,
                updated_at INTEGER
            );
            CREATE INDEX IF NOT EXISTS idx_peers_username ON peers(username);
        )";

        char* errmsg = nullptr;
        rc = sqlite3_exec(db, schema, nullptr, nullptr, &errmsg);
        if (rc != SQLITE_OK) {
            std::string err = errmsg ? errmsg : "Unknown schema execution error";
            sqlite3_free(errmsg);
            sqlite3_close(db);
            db = nullptr;
            throw StorageError("Failed to initialize SQLite tables: " + err);
        }
    }

    ~Impl() {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
    }
};

SqliteSessionStorage::SqliteSessionStorage(const std::string& db_path)
    : impl_(std::make_unique<Impl>(db_path)) {}

SqliteSessionStorage::~SqliteSessionStorage() = default;

void SqliteSessionStorage::set_value(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!impl_->db) return;

    const char* sql = "INSERT INTO kv_store(key, value, updated_at) VALUES(?, ?, strftime('%s', 'now')) "
                      "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = excluded.updated_at;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

std::optional<std::string> SqliteSessionStorage::get_value(const std::string& key) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!impl_->db) return std::nullopt;

    const char* sql = "SELECT value FROM kv_store WHERE key = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::optional<std::string> result = std::nullopt;

    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* val = sqlite3_column_text(stmt, 0);
            if (val) {
                result = std::string(reinterpret_cast<const char*>(val));
            }
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

void SqliteSessionStorage::delete_value(const std::string& key) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!impl_->db) return;

    const char* sql = "DELETE FROM kv_store WHERE key = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

void SqliteSessionStorage::save_peer(const PeerInfo& peer) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!impl_->db) return;

    const char* sql = "INSERT INTO peers(id, type, access_hash, username, phone, title, updated_at) "
                      "VALUES(?, ?, ?, ?, ?, ?, strftime('%s', 'now')) "
                      "ON CONFLICT(id) DO UPDATE SET type = excluded.type, access_hash = excluded.access_hash, "
                      "username = excluded.username, phone = excluded.phone, title = excluded.title, "
                      "updated_at = excluded.updated_at;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, peer.id);
        sqlite3_bind_int(stmt, 2, peer.type);
        sqlite3_bind_text(stmt, 3, peer.access_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, peer.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, peer.phone.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, peer.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

std::optional<PeerInfo> SqliteSessionStorage::get_peer(int64_t id) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!impl_->db) return std::nullopt;

    const char* sql = "SELECT id, type, access_hash, username, phone, title FROM peers WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::optional<PeerInfo> result = std::nullopt;

    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            PeerInfo p;
            p.id = sqlite3_column_int64(stmt, 0);
            p.type = sqlite3_column_int(stmt, 1);
            const auto* h = sqlite3_column_text(stmt, 2);
            if (h) p.access_hash = reinterpret_cast<const char*>(h);
            const auto* u = sqlite3_column_text(stmt, 3);
            if (u) p.username = reinterpret_cast<const char*>(u);
            const auto* ph = sqlite3_column_text(stmt, 4);
            if (ph) p.phone = reinterpret_cast<const char*>(ph);
            const auto* t = sqlite3_column_text(stmt, 5);
            if (t) p.title = reinterpret_cast<const char*>(t);
            result = p;
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::optional<PeerInfo> SqliteSessionStorage::get_peer_by_username(const std::string& username) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!impl_->db) return std::nullopt;

    const char* sql = "SELECT id, type, access_hash, username, phone, title FROM peers WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::optional<PeerInfo> result = std::nullopt;

    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            PeerInfo p;
            p.id = sqlite3_column_int64(stmt, 0);
            p.type = sqlite3_column_int(stmt, 1);
            const auto* h = sqlite3_column_text(stmt, 2);
            if (h) p.access_hash = reinterpret_cast<const char*>(h);
            const auto* u = sqlite3_column_text(stmt, 3);
            if (u) p.username = reinterpret_cast<const char*>(u);
            const auto* ph = sqlite3_column_text(stmt, 4);
            if (ph) p.phone = reinterpret_cast<const char*>(ph);
            const auto* t = sqlite3_column_text(stmt, 5);
            if (t) p.title = reinterpret_cast<const char*>(t);
            result = p;
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

void SqliteSessionStorage::close() {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (impl_->db) {
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
    }
}
#endif

std::shared_ptr<ISessionStorage> create_storage(const std::string& path) {
    if (path.empty() || path == ":memory:") {
        return std::make_shared<MemorySessionStorage>();
    }
#ifdef CPPGRAM_HAVE_SQLITE3
    return std::make_shared<SqliteSessionStorage>(path);
#else
    CPPGRAM_WARN("storage", "SQLite3 not available, falling back to in-memory session storage");
    return std::make_shared<MemorySessionStorage>();
#endif
}

} // namespace cppgram
