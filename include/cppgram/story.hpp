#pragma once

/**
 * @file story.hpp
 * @brief Domain models and privacy abstractions for Telegram Stories.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include "cppgram/types.hpp"
#include "cppgram/media.hpp"

namespace cppgram {

/**
 * @brief Target audience / visibility scope for stories.
 */
enum class StoryPrivacy {
    Public,
    Contacts,
    CloseFriends,
    SelectedUsers
};

/**
 * @brief Configures viewing permissions and recipient user lists for stories.
 */
struct StoryPrivacySettings {
    StoryPrivacy privacy{StoryPrivacy::Public};
    std::vector<UserId> allowed_user_ids;
    std::vector<UserId> disallowed_user_ids;
};

/**
 * @brief Represents an individual Telegram story entity.
 */
struct StoryItem {
    int32_t     id{0};
    ChatId      sender_chat_id{0};
    Timestamp   date{};
    bool        is_being_edited{false};
    bool        is_edited{false};
    bool        is_pinned{false};
    bool        is_visible_only_for_self{false};
    std::string caption;
    MediaType   media_type{MediaType::Photo};
    std::string media_file_id;
};

/**
 * @brief Collection of stories for a user or channel.
 */
struct Stories {
    int32_t total_count{0};
    std::vector<StoryItem> stories;
    std::string pinned_story_ids;
};

} // namespace cppgram
