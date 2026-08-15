#pragma once

/**
 * @file thread.hpp
 * @brief Message thread and forum topic entities for CppGram.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "cppgram/types.hpp"
#include "cppgram/message.hpp"

namespace cppgram {

/**
 * @brief Contains detailed information about a message thread or forum topic.
 */
struct MessageThreadInfo {
    ChatId    chat_id{0};
    int64_t   message_thread_id{0};
    int32_t   unread_message_count{0};
    std::vector<Message> messages;
    std::string draft_message;
};

/**
 * @brief Represents a forum topic in a supergroup chat.
 */
struct ForumTopic {
    int64_t     message_thread_id{0};
    std::string name;
    int32_t     icon_color{0};
    std::string icon_custom_emoji_id;
    bool        is_closed{false};
    bool        is_hidden{false};
};

} // namespace cppgram
