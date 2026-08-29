#pragma once

/**
 * @file types.hpp
 * @brief Common types, aliases, enumerations, and metadata structures for CppGram.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <functional>

namespace cppgram {

// ============================================================================
// Core Type Aliases
// ============================================================================

using ChatId    = std::int64_t;
using UserId    = std::int64_t;
using MessageId = std::int64_t;
using FileId    = std::int32_t;
using UpdateId  = std::int64_t;

using Timestamp = std::chrono::system_clock::time_point;

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @brief Represents the category/type of a Telegram chat.
 */
enum class ChatType { Private, BasicGroup, Supergroup, Channel, Secret, Unknown };

/**
 * @brief Represents the client authentication flow state.
 */
enum class AuthState {
    None, WaitPhoneNumber, WaitCode, WaitPassword,
    WaitRegistration, Ready, LoggingOut, Closed
};

/**
 * @brief Media classification for Telegram message attachments.
 */
enum class MediaType {
    None, Photo, Video, Document, Audio, Voice, VideoNote,
    Animation, Sticker, Poll, Location, Contact, Venue, Dice
};

/**
 * @brief Supported message text formatting parse modes.
 */
enum class ParseMode {
    None, Markdown, MarkdownV2, HTML
};

/**
 * @brief Message entity styling types.
 */
enum class MessageEntityType {
    Mention, Hashtag, Cashtag, BotCommand, Url, EmailAddress, PhoneNumber,
    Bold, Italic, Underline, Strikethrough, Spoiler, Code, Pre, PreCode,
    TextUrl, MentionName, CustomEmoji, Unknown
};

// ============================================================================
// Message Formatting Structures
// ============================================================================

/**
 * @brief Represents a formatted subsection of text (e.g. bold, link, code block).
 */
struct MessageEntity {
    MessageEntityType type{MessageEntityType::Unknown};
    int offset{0};
    int length{0};
    std::string argument;

    const std::string& url() const noexcept { return argument; }
};

/**
 * @brief Plain text paired with parsed formatting entity metadata.
 */
struct FormattedText {
    std::string text;
    std::vector<MessageEntity> entities;
};

/**
 * @brief Configuration parameters for message transmission.
 */
struct SendMessageOptions {
    ParseMode parse_mode{ParseMode::None};
    std::optional<Timestamp> schedule_date;
    std::optional<int64_t>   message_thread_id;
    bool disable_notification{false};
    bool from_background{false};
    bool protect_content{false};
};

// ============================================================================
// API & Client Configuration
// ============================================================================

/**
 * @brief Telegram application credentials required for MTProto/TDLib connectivity.
 */
struct ApiCredentials {
    std::int32_t api_id{};
    std::string  api_hash;
};

/**
 * @brief Supported client engine backends.
 */
enum class BackendType {
    TDLib,
    NativeMTProto
};

/**
 * @brief Configuration parameters for CppGram client instances.
 */
struct ClientConfig {
    std::int32_t api_id{0};
    std::string  api_hash;
    BackendType  backend{BackendType::TDLib};
    int          primary_dc{2};
    bool         test_mode{false};
    std::int32_t layer{225};
};

/**
 * @brief File transfer download progress indicator.
 */
struct FileProgress {
    std::int64_t downloaded{};
    std::int64_t total{};
    double ratio() const noexcept {
        return total > 0 ? static_cast<double>(downloaded) / total : 0.0;
    }
};

// ============================================================================
// Message Routing & Metadata Structures
// ============================================================================

/**
 * @brief Metadata for forwarded messages.
 */
struct ForwardInfo {
    UserId      origin_sender_id{};
    ChatId      origin_chat_id{};
    MessageId   origin_message_id{};
    std::string origin_sender_name;
    Timestamp   origin_date{};
};

/**
 * @brief Metadata for message reply references.
 */
struct ReplyInfo {
    ChatId    reply_in_chat_id{};
    MessageId reply_to_message_id{};
};

/**
 * @brief Geographic coordinates and live location metadata.
 */
struct Location {
    double latitude{};
    double longitude{};
    double horizontal_accuracy{};
    std::int32_t live_period{};
    std::int32_t heading{};
    std::int32_t proximity_alert_radius{};
};

/**
 * @brief User phone contact entry.
 */
struct Contact {
    std::string phone_number;
    std::string first_name;
    std::string last_name;
    UserId user_id{};
    std::string vcard;
};

/**
 * @brief Point of interest / venue metadata.
 */
struct Venue {
    Location location;
    std::string title;
    std::string address;
    std::string provider;
    std::string id;
    std::string type;
};

// ============================================================================
// Polls & Interactive Elements
// ============================================================================

/**
 * @brief Single option within a poll.
 */
struct PollOption {
    std::string text;
    int voter_count{};
};

/**
 * @brief Category of poll (standard voting vs educational quiz).
 */
enum class PollType { Regular, Quiz };

/**
 * @brief Configuration settings for creating a new poll.
 */
struct PollConfig {
    PollType type{PollType::Regular};
    bool is_anonymous{true};
    bool allows_multiple_answers{false};
    std::optional<int> correct_option_id;
    std::string explanation;
    std::int32_t open_period{};
};

/**
 * @brief Poll payload and current voting statistics.
 */
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

/**
 * @brief Animated dice roll payload and outcome value.
 */
struct Dice {
    std::string emoji;
    int value{};
};

// ============================================================================
// Chat Permissions & Administration
// ============================================================================

/**
 * @brief Granular user permissions within a group or supergroup chat.
 */
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

/**
 * @brief Granular administrator privileges within a chat.
 */
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

/**
 * @brief Status of a participant within a chat.
 */
enum class ChatMemberStatus {
    Creator, Administrator, Member, Restricted, Left, Banned
};

/**
 * @brief File descriptor for local upload payloads.
 */
struct InputFile {
    std::string path;

    static InputFile local(const std::string& p) { return {p}; }
};

// ============================================================================
// Telegram API Layer 225 Domain Entities
// ============================================================================

/**
 * @brief Custom AI composer tone descriptor (Layer 225).
 */
struct AiComposeTone {
    int64_t     id{0};
    std::string title;
    std::string prompt;
    int64_t     emoji_id{0};
    bool        display_author{false};
    bool        is_default{false};
    std::string slug;
};

/**
 * @brief Bot Guest Chat response descriptor (Layer 225).
 */
struct BotGuestChatResult {
    int64_t     query_id{0};
    std::string text;
    ParseMode   parse_mode{ParseMode::None};
};

/**
 * @brief Poll statistics metadata (Layer 225).
 */
struct PollStats {
    int32_t total_voters{0};
    std::vector<int32_t> option_voters;
};

} // namespace cppgram
