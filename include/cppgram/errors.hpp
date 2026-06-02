#pragma once
#include <stdexcept>
#include <string>
#include <cstdint>
#include <chrono>

namespace cppgram {

enum class ErrorCode {
    Unknown,
    Authentication,
    Network,
    FloodWait,
    Rpc,
    Storage,
    InvalidArgument,
    Timeout,
    SeeOther,
    BadRequest,
    Unauthorized,
    Forbidden,
    NotFound,
    NotAcceptable,
    Flood,
    InternalServer
};

// ---- Exception hierarchy (public synchronous API) ------------------------
class CppGramException : public std::runtime_error {
public:
    explicit CppGramException(std::string msg, ErrorCode code = ErrorCode::Unknown)
        : std::runtime_error(std::move(msg)), code_(code) {}
    ErrorCode code() const noexcept { return code_; }
private:
    ErrorCode code_;
};

class AuthenticationError : public CppGramException {
public: explicit AuthenticationError(std::string m)
    : CppGramException(std::move(m), ErrorCode::Authentication) {}
};

class NetworkError : public CppGramException {
public: explicit NetworkError(std::string m)
    : CppGramException(std::move(m), ErrorCode::Network) {}
};

class RPCError : public CppGramException {
public:
    RPCError(std::int32_t rpc_code, std::string m)
        : CppGramException(std::move(m), ErrorCode::Rpc), rpc_code_(rpc_code) {}
    std::int32_t rpc_code() const noexcept { return rpc_code_; }
private:
    std::int32_t rpc_code_;
};

// ---- 303 See Other Errors ----
class SeeOtherError : public RPCError {
public:
    SeeOtherError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

class MigrationError : public SeeOtherError {
public:
    MigrationError(std::int32_t rpc_code, std::int32_t dc, std::string m)
        : SeeOtherError(rpc_code, std::move(m)), dc_(dc) {}
    std::int32_t dc() const noexcept { return dc_; }
private:
    std::int32_t dc_;
};

class PhoneMigrateError : public MigrationError {
public:
    PhoneMigrateError(std::int32_t dc, std::string m)
        : MigrationError(303, dc, std::move(m)) {}
};

class FileMigrateError : public MigrationError {
public:
    FileMigrateError(std::int32_t dc, std::string m)
        : MigrationError(303, dc, std::move(m)) {}
};

class UserMigrateError : public MigrationError {
public:
    UserMigrateError(std::int32_t dc, std::string m)
        : MigrationError(303, dc, std::move(m)) {}
};

// ---- 400 Bad Request Errors ----
class BadRequestError : public RPCError {
public:
    BadRequestError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

// ---- 401 Unauthorized Errors ----
class UnauthorizedError : public RPCError {
public:
    UnauthorizedError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

// ---- 403 Forbidden Errors ----
class ForbiddenError : public RPCError {
public:
    ForbiddenError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

// ---- 404 Not Found Errors ----
class NotFoundError : public RPCError {
public:
    NotFoundError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

// ---- 406 Not Acceptable Errors ----
class NotAcceptableError : public RPCError {
public:
    NotAcceptableError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

// ---- 420 Flood Errors ----
class FloodError : public RPCError {
public:
    FloodError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

class FloodWaitError : public FloodError {
public:
    FloodWaitError(std::int32_t seconds, std::string m)
        : FloodError(420, std::move(m)), seconds_(seconds) {}
    std::int32_t seconds() const noexcept { return seconds_; }
    std::chrono::seconds retry_after() const noexcept { return std::chrono::seconds(seconds_); }
private:
    std::int32_t seconds_;
};

class SlowmodeWaitError : public FloodError {
public:
    SlowmodeWaitError(std::int32_t seconds, std::string m)
        : FloodError(420, std::move(m)), seconds_(seconds) {}
    std::int32_t seconds() const noexcept { return seconds_; }
private:
    std::int32_t seconds_;
};

// ---- 500 Internal Server Errors ----
class InternalServerError : public RPCError {
public:
    InternalServerError(std::int32_t rpc_code, std::string m)
        : RPCError(rpc_code, std::move(m)) {}
};

class StorageError : public CppGramException {
public: explicit StorageError(std::string m)
    : CppGramException(std::move(m), ErrorCode::Storage) {}
};

// ---- Non-throwing error carrier used by Result<T> ------------------------
struct Error {
    ErrorCode    code = ErrorCode::Unknown;
    std::string  message;
    std::int32_t rpc_code = 0;      // for RPC errors
    std::int32_t retry_after = 0;   // for FloodWait / SlowmodeWait
    std::int32_t migrate_to_dc = 0; // for MigrationError

    static Error from_rpc(std::int32_t rpc_code, std::string message) {
        Error err;
        err.rpc_code = rpc_code;
        err.message = message;

        switch (rpc_code) {
            case 303: {
                err.code = ErrorCode::SeeOther;
                auto pos = message.find("PHONE_MIGRATE_");
                if (pos != std::string::npos) {
                    try { err.migrate_to_dc = std::stoi(message.substr(pos + 14)); } catch (...) {}
                } else {
                    pos = message.find("FILE_MIGRATE_");
                    if (pos != std::string::npos) {
                        try { err.migrate_to_dc = std::stoi(message.substr(pos + 13)); } catch (...) {}
                    } else {
                        pos = message.find("USER_MIGRATE_");
                        if (pos != std::string::npos) {
                            try { err.migrate_to_dc = std::stoi(message.substr(pos + 13)); } catch (...) {}
                        }
                    }
                }
                break;
            }
            case 400: err.code = ErrorCode::BadRequest; break;
            case 401: err.code = ErrorCode::Unauthorized; break;
            case 403: err.code = ErrorCode::Forbidden; break;
            case 404: err.code = ErrorCode::NotFound; break;
            case 406: err.code = ErrorCode::NotAcceptable; break;
            case 420: {
                auto pos = message.find("FLOOD_WAIT_");
                if (pos != std::string::npos) {
                    err.code = ErrorCode::FloodWait;
                    try {
                        err.retry_after = std::stoi(message.substr(pos + 11));
                    } catch (...) {}
                } else {
                    pos = message.find("SLOWMODE_WAIT_");
                    if (pos != std::string::npos) {
                        err.code = ErrorCode::Flood;
                        try {
                            err.retry_after = std::stoi(message.substr(pos + 14));
                        } catch (...) {}
                    } else {
                        err.code = ErrorCode::Flood;
                    }
                }
                break;
            }
            case 500: err.code = ErrorCode::InternalServer; break;
            default:  err.code = ErrorCode::Rpc; break;
        }
        return err;
    }

    [[noreturn]] void raise() const {
        switch (code) {
            case ErrorCode::Authentication: throw AuthenticationError(message);
            case ErrorCode::Network:        throw NetworkError(message);
            case ErrorCode::FloodWait:      throw FloodWaitError(retry_after, message);
            case ErrorCode::Storage:        throw StorageError(message);
            case ErrorCode::SeeOther: {
                if (message.find("PHONE_MIGRATE_") != std::string::npos) {
                    throw PhoneMigrateError(migrate_to_dc, message);
                } else if (message.find("FILE_MIGRATE_") != std::string::npos) {
                    throw FileMigrateError(migrate_to_dc, message);
                } else if (message.find("USER_MIGRATE_") != std::string::npos) {
                    throw UserMigrateError(migrate_to_dc, message);
                } else {
                    throw SeeOtherError(rpc_code, message);
                }
            }
            case ErrorCode::BadRequest:     throw BadRequestError(rpc_code, message);
            case ErrorCode::Unauthorized:   throw UnauthorizedError(rpc_code, message);
            case ErrorCode::Forbidden:      throw ForbiddenError(rpc_code, message);
            case ErrorCode::NotFound:       throw NotFoundError(rpc_code, message);
            case ErrorCode::NotAcceptable:  throw NotAcceptableError(rpc_code, message);
            case ErrorCode::Flood: {
                if (message.find("SLOWMODE_WAIT_") != std::string::npos) {
                    throw SlowmodeWaitError(retry_after, message);
                } else {
                    throw FloodError(rpc_code, message);
                }
            }
            case ErrorCode::InternalServer: throw InternalServerError(rpc_code, message);
            case ErrorCode::Rpc:            throw RPCError(rpc_code, message);
            default:                        throw CppGramException(message, code);
        }
    }
};

} // namespace cppgram