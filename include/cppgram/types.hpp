#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace cppgram {

using ChatId    = std::int64_t;
using UserId    = std::int64_t;
using MessageId = std::int64_t;
using FileId    = std::int32_t;
using UpdateId  = std::int64_t;

enum class ChatType { Private, BasicGroup, Supergroup, Channel, Secret, Unknown };

enum class AuthState {
    None, WaitPhoneNumber, WaitCode, WaitPassword,
    WaitRegistration, Ready, LoggingOut, Closed
};

enum class MediaType { None, Photo, Video, Document, Audio, Voice, Animation, Sticker };

struct ApiCredentials {
    std::int32_t api_id{};
    std::string  api_hash;
};

// Progress callback shared by upload/download (Phase 6).
struct FileProgress {
    std::int64_t downloaded{};
    std::int64_t total{};
    double ratio() const noexcept {
        return total > 0 ? static_cast<double>(downloaded) / total : 0.0;
    }
};

using Timestamp = std::chrono::system_clock::time_point;

} // namespace cppgram