#include "cppgram/message.hpp"
#include "cppgram/chat.hpp"
#include "cppgram/client.hpp"
#include "cppgram/log.hpp"

namespace cppgram {

Message Message::reply(const std::string& body) {
    if (auto c = _client.lock())
        return c->sendMessage(chat_id, body, /*reply_to=*/id);
    CPPGRAM_WARN("entity", "Message::reply on expired client");
    return {};
}
void Message::edit(const std::string& body) {
    if (auto c = _client.lock()) c->editMessage(chat_id, id, body);
}
void Message::deleteMessage() {
    if (auto c = _client.lock()) c->deleteMessages(chat_id, {id});
}
void Message::react(const std::string& emoji) {
    if (auto c = _client.lock()) c->setReaction(chat_id, id, emoji);
}

Message Chat::sendMessage(const std::string& body) {
    if (auto c = _client.lock()) return c->sendMessage(id, body, std::nullopt);
    return {};
}
void Chat::leave()   { if (auto c = _client.lock()) c->leaveChat(id); }
void Chat::join()    { if (auto c = _client.lock()) c->joinChat(id); }
std::vector<User> Chat::members() {
    if (auto c = _client.lock()) return c->getChatMembers(id);
    return {};
}

} // namespace cppgram