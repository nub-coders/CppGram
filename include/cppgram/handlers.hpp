#pragma once
#include <functional>
#include <optional>
#include "cppgram/message.hpp"
#include "cppgram/callback_query.hpp"
#include "cppgram/filters.hpp"

namespace cppgram {

struct MessageHandler {
    MessageFilter                filter{[](const Message&) { return true; }};
    std::function<void(Message)> callback;
};

struct EditedMessageHandler {
    MessageFilter                filter{[](const Message&) { return true; }};
    std::function<void(Message)> callback;
};

struct DeletedMessagesHandler {
    std::function<void(ChatId, std::vector<MessageId>)> callback;
};

struct CallbackQueryHandler {
    std::function<bool(const CallbackQuery&)> filter;
    std::function<void(CallbackQuery)>        callback;
};

} // namespace cppgram
