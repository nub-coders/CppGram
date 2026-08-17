#pragma once

/**
 * @file web_app.hpp
 * @brief Telegram Mini Apps (Web Apps) models and query structures.
 */

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace cppgram {

/**
 * @brief Mode for opening a Telegram Mini App.
 */
enum class WebAppOpenMode {
    Compact,
    Tall,
    FullScreen
};

/**
 * @brief Describes a Telegram Mini App web application.
 */
struct WebAppInfo {
    std::string url;
};

/**
 * @brief Contains data sent from a Web App back to the bot.
 */
struct WebAppData {
    std::string data;
    std::string button_text;
};

/**
 * @brief Information about a message sent on behalf of a user via a Mini App.
 */
struct SentWebAppMessage {
    std::optional<std::string> inline_message_id;
};

} // namespace cppgram
