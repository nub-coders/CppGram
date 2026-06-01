#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <functional>

namespace cppgram {

// ---- Core type aliases ----
using ChatId    = std::int64_t;
using UserId    = std::int64_t;
using MessageId = std::int64_t;
using FileId    = std::int32_t;
using UpdateId  = std::int64_t;

using Timestamp = std::chrono::system_clock::time_point;

// ---- Enums ----
enum class ChatType { Private, BasicGroup, Supergroup, Channel, Secret, Unknown };

enum class AuthState {
    None, WaitPhoneNumber, WaitCode, WaitPassword,
    WaitRegistration, Ready, LoggingOut, Closed
};

enum class MediaType {
    None, Photo, Video, Document, Audio, Voice, VideoNote,
    Animation, Sticker, Poll, Location, Contact, Venue, Dice
};

// ---- API credentials ----
struct ApiCredentials {
    std::int32_t api_id{};
    std::string  api_hash;
};

// ---- File progress ----
struct FileProgress {
    std::int64_t downloaded{};
    std::int64_t total{};
    double ratio() const noexcept {
        return total > 0 ? static_cast<double>(downloaded) / total : 0.0;
    }
};

// ---- Forward info ----
struct ForwardInfo {
    UserId      origin_sender_id{};
    ChatId      origin_chat_id{};
    MessageId   origin_message_id{};
    std::string origin_sender_name;
    Timestamp   origin_date{};
};

// ---- Reply info ----
struct ReplyInfo {
    ChatId    reply_in_chat_id{};
    MessageId reply_to_message_id{};
};

// ---- Location ----
struct Location {
    double latitude{};
    double longitude{};
    double horizontal_accuracy{};
    std::int32_t live_period{};
    std::int32_t heading{};
    std::int32_t proximity_alert_radius{};
};

// ---- Contact ----
struct Contact {
    std::string phone_number;
    std::string first_name;
    std::string last_name;
    UserId user_id{};
    std::string vcard;
};

// ---- Venue ----
struct Venue {
    Location location;
    std::string title;
    std::string address;
    std::string provider;
    std::string id;
    std::string type;
};

// ---- Poll ----
struct PollOption {
    std::string text;
    int voter_count{};
};

enum class PollType { Regular, Quiz };

struct PollConfig {
    PollType type{PollType::Regular};
    bool is_anonymous{true};
    bool allows_multiple_answers{false};
    std::optional<int> correct_option_id;
    std::string explanation;
    std::int32_t open_period{};
};

struct Poll {
    std::int64_t id{};
    std::string question;
    std::vector<PollOption> options;
    int total_voter_count{};
    bool is_closed{};
    bool is_anonymous{true};
    PollType type{PollType::Regular};
    bool allows_multiple_answers{false};
    std::optional<int> correct_option_id;
    std::string explanation;
};

// ---- Dice ----
struct Dice {
    std::string emoji;
    int value{};
};

// ---- Chat permissions ----
struct ChatPermissions {
    bool can_send_messages{true};
    bool can_send_media{true};
    bool can_send_polls{true};
    bool can_send_other{true};
    bool can_add_web_page_previews{true};
    bool can_change_info{false};
    bool can_invite_users{true};
    bool can_pin_messages{false};
    bool can_manage_topics{false};
};

// ---- Chat admin rights ----
struct ChatAdminRights {
    bool can_manage_chat{false};
    bool can_change_info{false};
    bool can_post_messages{false};
    bool can_edit_messages{false};
    bool can_delete_messages{false};
    bool can_invite_users{false};
    bool can_restrict_members{false};
    bool can_pin_messages{false};
    bool can_promote_members{false};
    bool can_manage_video_chats{false};
    bool is_anonymous{false};
    std::string custom_title;
};

// ---- Chat member status ----
enum class ChatMemberStatus {
    Creator, Administrator, Member, Restricted, Left, Banned
};

// ---- Input file for uploads ----
struct InputFile {
    std::string path;

    static InputFile local(const std::string& p) { return {p}; }
};

} // namespace cppgram
