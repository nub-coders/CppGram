#pragma once

/**
 * @file business.hpp
 * @brief Domain models and structures for Telegram Business connections and Quick Replies.
 */

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"

namespace cppgram {

/**
 * @brief Opening hours interval for a Telegram Business account.
 */
struct BusinessOpeningHoursInterval {
    int32_t opening_minute{0};
    int32_t closing_minute{0};
};

/**
 * @brief Opening hours for a Telegram Business account.
 */
struct BusinessOpeningHours {
    std::string time_zone_name;
    std::vector<BusinessOpeningHoursInterval> opening_hours;
};

/**
 * @brief Physical location of a Telegram Business account.
 */
struct BusinessLocation {
    std::string address;
    std::optional<Location> location;
};

/**
 * @brief Intro information displayed on a business account profile.
 */
struct BusinessIntro {
    std::string title;
    std::string message;
    std::string sticker_file_id;
};

/**
 * @brief Describes the connection of a bot with a Telegram Business account.
 */
struct BusinessConnection {
    std::string id;
    User        user;
    UserId      user_chat_id{0};
    Timestamp   date{};
    bool        can_reply{false};
    bool        is_enabled{false};
};

/**
 * @brief Represents a quick reply shortcut for business accounts.
 */
struct QuickReplyShortcut {
    int32_t     id{0};
    std::string name;
    int32_t     message_count{0};
};

/**
 * @brief Represents messages received from or sent to a connected business account.
 */
struct BusinessMessages {
    std::string business_connection_id;
    std::vector<MessageId> message_ids;
};

} // namespace cppgram
