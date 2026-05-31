#pragma once
#include <string>
#include <memory>
#include <optional>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"

namespace cppgram {
class ClientImpl;

class Message {
public:
    MessageId   id{};
    ChatId      chat_id{};
    std::string text;
    User        sender;
    Timestamp   date{};
    bool        outgoing{false};
    MediaType   media{MediaType::None};
    std::optional<MessageId> reply_to;

    Message reply(const std::string& text);
    void    edit(const std::string& text);
    void    deleteMessage();
    void    react(const std::string& emoji);

    bool has_media() const noexcept { return media != MediaType::None; }

    std::weak_ptr<ClientImpl> _client;
};
} // namespace cppgram