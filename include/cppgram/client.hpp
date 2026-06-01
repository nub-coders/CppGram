#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"
#include "cppgram/chat.hpp"
#include "cppgram/message.hpp"
#include "cppgram/media.hpp"
#include "cppgram/keyboard.hpp"
#include "cppgram/callback_query.hpp"
#include "cppgram/handlers.hpp"

namespace cppgram {
class ClientImpl;

class Client {
public:
    Client();
    Client(std::int32_t api_id, std::string api_hash);
    ~Client();

    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // ---- Auth ----
    void login(const std::string& phone,
               std::function<std::string()> code_callback = {},
               std::function<std::string()> password_callback = {});
    void loginBot(const std::string& token);
    void logout();
    AuthState authState() const;

    // ---- Users & chats ----
    User              getMe();
    User              getUser(UserId id);
    Chat              getChat(ChatId id);
    std::vector<Message> getHistory(ChatId chat_id, int limit = 100);

    // ---- Text messaging ----
    Message sendMessage(ChatId chat_id, const std::string& text,
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendMessage(ChatId chat_id, const std::string& text,
                        const ReplyMarkup& markup,
                        std::optional<MessageId> reply_to = std::nullopt);
    void    editMessage(ChatId, MessageId, const std::string& text);
    void    editMessageCaption(ChatId, MessageId, const std::string& caption);
    void    editMessageReplyMarkup(ChatId, MessageId, const InlineKeyboard& markup);
    void    deleteMessages(ChatId, std::vector<MessageId>);
    void    setReaction(ChatId, MessageId, const std::string& emoji);
    Message forwardMessage(ChatId from_chat, MessageId msg_id, ChatId to_chat);
    void    pinMessage(ChatId, MessageId, bool disable_notification = false);
    void    unpinMessage(ChatId, MessageId);
    void    unpinAllMessages(ChatId);

    // ---- Media messaging ----
    Message sendPhoto(ChatId, const InputFile&,
                      const std::string& caption = "",
                      std::optional<MessageId> reply_to = std::nullopt);
    Message sendVideo(ChatId, const InputFile&,
                      const std::string& caption = "",
                      std::optional<MessageId> reply_to = std::nullopt,
                      int width = 0, int height = 0, int duration = 0);
    Message sendDocument(ChatId, const InputFile&,
                         const std::string& caption = "",
                         std::optional<MessageId> reply_to = std::nullopt);
    Message sendAudio(ChatId, const InputFile&,
                      const std::string& caption = "",
                      std::optional<MessageId> reply_to = std::nullopt,
                      int duration = 0,
                      const std::string& title = "",
                      const std::string& performer = "");
    Message sendVoiceNote(ChatId, const InputFile&,
                          const std::string& caption = "",
                          std::optional<MessageId> reply_to = std::nullopt,
                          int duration = 0);
    Message sendVideoNote(ChatId, const InputFile&,
                          std::optional<MessageId> reply_to = std::nullopt,
                          int duration = 0, int length = 0);
    Message sendAnimation(ChatId, const InputFile&,
                          const std::string& caption = "",
                          std::optional<MessageId> reply_to = std::nullopt,
                          int width = 0, int height = 0, int duration = 0);
    Message sendSticker(ChatId, const InputFile&,
                        std::optional<MessageId> reply_to = std::nullopt);

    // ---- Rich messages ----
    Message sendPoll(ChatId, const std::string& question,
                     std::vector<std::string> options,
                     const PollConfig& config = {});
    Message sendDice(ChatId, const std::string& emoji = "\xF0\x9F\x8E\xB2");
    Message sendContact(ChatId, const Contact&);
    Message sendLocation(ChatId, const Location&);
    Message sendVenue(ChatId, const Venue&);
    void    stopPoll(ChatId, MessageId);

    // ---- Chat management ----
    void              leaveChat(ChatId);
    void              joinChat(ChatId);
    std::vector<User> getChatMembers(ChatId);
    ChatId createGroup(const std::string& title, std::vector<UserId> members);
    ChatId createSupergroup(const std::string& title, bool is_channel = false,
                            const std::string& description = "");
    void setChatTitle(ChatId, const std::string& title);
    void setChatDescription(ChatId, const std::string& description);
    void setChatPhoto(ChatId, const InputFile&);
    void deleteChatPhoto(ChatId);
    void setChatPermissions(ChatId, const ChatPermissions&);
    void banChatMember(ChatId, UserId);
    void unbanChatMember(ChatId, UserId);
    void restrictChatMember(ChatId, UserId, const ChatPermissions&);
    void promoteChatMember(ChatId, UserId, const ChatAdminRights&);
    ChatMember getChatMember(ChatId, UserId);
    int getChatMemberCount(ChatId);
    std::vector<ChatMember> getChatAdministrators(ChatId);
    std::string getChatInviteLink(ChatId);

    // ---- User operations ----
    void blockUser(UserId);
    void unblockUser(UserId);

    // ---- File operations ----
    FileInfo    getFile(FileId);
    std::string downloadFile(FileId, const std::string& directory = ".",
                             std::function<void(FileProgress)> progress = {});

    // ---- Search ----
    std::vector<Message> searchMessages(const std::string& query, int limit = 100);
    std::vector<Message> searchChatMessages(ChatId, const std::string& query,
                                            int limit = 100);

    // ---- Callback queries ----
    void answerCallbackQuery(std::int64_t query_id,
                             const std::string& text = "",
                             bool show_alert = false,
                             const std::string& url = "",
                             int cache_time = 0);

    // ---- Event handlers ----
    using MessageCallback = std::function<void(Message)>;

    void onMessage(MessageCallback cb);
    void onMessage(MessageFilter filter, MessageCallback cb);
    void onEditedMessage(MessageCallback cb);
    void onEditedMessage(MessageFilter filter, MessageCallback cb);
    void onDeletedMessages(std::function<void(ChatId, std::vector<MessageId>)> cb);
    void onCallbackQuery(std::function<void(CallbackQuery)> cb);
    void onCallbackQuery(std::function<bool(const CallbackQuery&)> filter,
                         std::function<void(CallbackQuery)> cb);

    // ---- Event loop ----
    void run();
    void stop();

private:
    std::shared_ptr<ClientImpl> impl_;
};

} // namespace cppgram
