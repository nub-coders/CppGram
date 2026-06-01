#pragma once
#include <string>
#include <memory>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"

namespace cppgram {
class IBackend;

struct CallbackQuery {
    std::int64_t id{};
    User sender;
    ChatId chat_id{};
    MessageId message_id{};
    std::string data;
    std::string chat_instance;

    void answer(const std::string& text = "", bool show_alert = false);

    std::weak_ptr<IBackend> _client;
};

} // namespace cppgram
