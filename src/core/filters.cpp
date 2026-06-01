#include "cppgram/filters.hpp"
#include <algorithm>
#include <regex>

namespace cppgram::Filters {

MessageFilter command(const std::string& cmd) {
    return [cmd](const Message& m) -> bool {
        if (m.text.empty() || m.text[0] != '/') return false;
        auto end = m.text.find_first_of(" @", 1);
        std::string found = m.text.substr(
            1, end == std::string::npos ? end : end - 1);
        return found == cmd;
    };
}

MessageFilter privateChat() {
    return [](const Message& m) { return m.chat_type == ChatType::Private; };
}

MessageFilter group() {
    return [](const Message& m) {
        return m.chat_type == ChatType::BasicGroup ||
               m.chat_type == ChatType::Supergroup;
    };
}

MessageFilter channel() {
    return [](const Message& m) { return m.chat_type == ChatType::Channel; };
}

MessageFilter text() {
    return [](const Message& m) { return !m.text.empty(); };
}

MessageFilter media() {
    return [](const Message& m) { return m.has_media(); };
}

MessageFilter photo() {
    return [](const Message& m) { return m.media == MediaType::Photo; };
}

MessageFilter video() {
    return [](const Message& m) { return m.media == MediaType::Video; };
}

MessageFilter document() {
    return [](const Message& m) { return m.media == MediaType::Document; };
}

MessageFilter audio() {
    return [](const Message& m) { return m.media == MediaType::Audio; };
}

MessageFilter voice() {
    return [](const Message& m) { return m.media == MediaType::Voice; };
}

MessageFilter videoNote() {
    return [](const Message& m) { return m.media == MediaType::VideoNote; };
}

MessageFilter animation() {
    return [](const Message& m) { return m.media == MediaType::Animation; };
}

MessageFilter sticker() {
    return [](const Message& m) { return m.media == MediaType::Sticker; };
}

MessageFilter pollMsg() {
    return [](const Message& m) { return m.media == MediaType::Poll; };
}

MessageFilter locationMsg() {
    return [](const Message& m) { return m.media == MediaType::Location; };
}

MessageFilter contactMsg() {
    return [](const Message& m) { return m.media == MediaType::Contact; };
}

MessageFilter dice() {
    return [](const Message& m) { return m.media == MediaType::Dice; };
}

MessageFilter bot() {
    return [](const Message& m) { return m.sender.is_bot; };
}

MessageFilter outgoing() {
    return [](const Message& m) { return m.outgoing; };
}

MessageFilter incoming() {
    return [](const Message& m) { return !m.outgoing; };
}

MessageFilter reply() {
    return [](const Message& m) { return m.reply_to.has_value(); };
}

MessageFilter forwarded() {
    return [](const Message& m) { return m.forward_info.has_value(); };
}

MessageFilter regex(const std::string& pattern) {
    auto re = std::make_shared<std::regex>(pattern);
    return [re](const Message& m) {
        return std::regex_search(m.text, *re);
    };
}

MessageFilter chatId(ChatId id) {
    return [id](const Message& m) { return m.chat_id == id; };
}

MessageFilter userId(UserId id) {
    return [id](const Message& m) { return m.sender.id == id; };
}

} // namespace cppgram::Filters
