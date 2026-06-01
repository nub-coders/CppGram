#include "cppgram/message.hpp"
#include "cppgram/chat.hpp"
#include "cppgram/callback_query.hpp"
#include "cppgram/i_backend.hpp"
#include "cppgram/log.hpp"

namespace cppgram {

// ---- Message methods ----

Message Message::reply(const std::string& body) {
    if (auto c = _client.lock())
        return c->sendMessage(chat_id, body, id);
    CPPGRAM_WARN("entity", "Message::reply on expired client");
    return {};
}

Message Message::reply(const std::string& body, const ReplyMarkup& markup) {
    if (auto c = _client.lock())
        return c->sendMessageWithMarkup(chat_id, body, id, markup);
    CPPGRAM_WARN("entity", "Message::reply on expired client");
    return {};
}

void Message::edit(const std::string& body) {
    if (auto c = _client.lock()) c->editMessage(chat_id, id, body);
}

void Message::editCaption(const std::string& new_caption) {
    if (auto c = _client.lock()) c->editMessageCaption(chat_id, id, new_caption);
}

void Message::editReplyMarkup(const InlineKeyboard& markup) {
    if (auto c = _client.lock()) c->editMessageReplyMarkup(chat_id, id, markup);
}

void Message::deleteMessage() {
    if (auto c = _client.lock()) c->deleteMessages(chat_id, {id});
}

void Message::react(const std::string& emoji) {
    if (auto c = _client.lock()) c->setReaction(chat_id, id, emoji);
}

Message Message::forward(ChatId target_chat) {
    if (auto c = _client.lock())
        return c->forwardMessage(chat_id, id, target_chat);
    CPPGRAM_WARN("entity", "Message::forward on expired client");
    return {};
}

void Message::pin(bool disable_notification) {
    if (auto c = _client.lock()) c->pinMessage(chat_id, id, disable_notification);
}

void Message::unpin() {
    if (auto c = _client.lock()) c->unpinMessage(chat_id, id);
}

// ---- Chat methods ----

Message Chat::sendMessage(const std::string& body) {
    if (auto c = _client.lock()) return c->sendMessage(id, body, std::nullopt);
    return {};
}

void Chat::leave()  { if (auto c = _client.lock()) c->leaveChat(id); }
void Chat::join()   { if (auto c = _client.lock()) c->joinChat(id); }

std::vector<User> Chat::members() {
    if (auto c = _client.lock()) return c->getChatMembers(id);
    return {};
}

void Chat::setTitle(const std::string& new_title) {
    if (auto c = _client.lock()) c->setChatTitle(id, new_title);
}

void Chat::setDescription(const std::string& desc) {
    if (auto c = _client.lock()) c->setChatDescription(id, desc);
}

void Chat::setPermissions(const ChatPermissions& perms) {
    if (auto c = _client.lock()) c->setChatPermissions(id, perms);
}

void Chat::banMember(UserId user_id) {
    if (auto c = _client.lock()) c->banChatMember(id, user_id);
}

void Chat::unbanMember(UserId user_id) {
    if (auto c = _client.lock()) c->unbanChatMember(id, user_id);
}

void Chat::promoteMember(UserId user_id, const ChatAdminRights& rights) {
    if (auto c = _client.lock()) c->promoteChatMember(id, user_id, rights);
}

void Chat::restrictMember(UserId user_id, const ChatPermissions& perms) {
    if (auto c = _client.lock()) c->restrictChatMember(id, user_id, perms);
}

std::string Chat::getInviteLink() {
    if (auto c = _client.lock()) return c->getChatInviteLink(id);
    return "";
}

int Chat::getMemberCount() {
    if (auto c = _client.lock()) return c->getChatMemberCount(id);
    return 0;
}

// ---- CallbackQuery methods ----

void CallbackQuery::answer(const std::string& text, bool show_alert) {
    if (auto c = _client.lock())
        c->answerCallbackQuery(id, text, show_alert, "", 0);
}

} // namespace cppgram
