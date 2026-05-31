#pragma once
#include <stdexcept>
#include <string>
#include <cstdint>
#include <chrono>

namespace cppgram {

enum class ErrorCode {
    Unknown, Authentication, Network, FloodWait, Rpc, Storage, InvalidArgument, Timeout
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

class FloodWaitError : public CppGramException {
public:
    FloodWaitError(std::int32_t seconds, std::string m)
        : CppGramException(std::move(m), ErrorCode::FloodWait), seconds_(seconds) {}
    std::int32_t seconds() const noexcept { return seconds_; }
    std::chrono::seconds retry_after() const noexcept { return std::chrono::seconds(seconds_); }
private:
    std::int32_t seconds_;
};

class RPCError : public CppGramException {
public:
    RPCError(std::int32_t rpc_code, std::string m)
        : CppGramException(std::move(m), ErrorCode::Rpc), rpc_code_(rpc_code) {}
    std::int32_t rpc_code() const noexcept { return rpc_code_; }
private:
    std::int32_t rpc_code_;
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
    std::int32_t retry_after = 0;   // for FloodWait

    [[noreturn]] void raise() const {
        switch (code) {
            case ErrorCode::Authentication: throw AuthenticationError(message);
            case ErrorCode::Network:        throw NetworkError(message);
            case ErrorCode::FloodWait:      throw FloodWaitError(retry_after, message);
            case ErrorCode::Rpc:            throw RPCError(rpc_code, message);
            case ErrorCode::Storage:        throw StorageError(message);
            default:                        throw CppGramException(message, code);
        }
    }
};

} // namespace cppgram