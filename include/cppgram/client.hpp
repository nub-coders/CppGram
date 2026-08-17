#pragma once

/**
 * @file client.hpp
 * @brief Primary Client interface for CppGram Telegram framework.
 */

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
#include "cppgram/storage.hpp"
#include "cppgram/coro.hpp"
#include "cppgram/thread.hpp"
#include "cppgram/story.hpp"
#include "cppgram/plugin.hpp"
#include "cppgram/middleware.hpp"
#include "cppgram/thread_pool.hpp"
#include "cppgram/web_app.hpp"
#include "cppgram/business.hpp"
#include "cppgram/secret_chat.hpp"
#include "cppgram/call.hpp"
#include "cppgram/transport.hpp"

namespace cppgram {
class ClientImpl;

/**
 * @brief High-level Telegram client supporting MTProto, TDLib, bots, user accounts, coroutines, and plugins.
 */
class Client {
public:
    Client();
    Client(std::int32_t api_id, std::string api_hash);
    Client(std::int32_t api_id, std::string api_hash, std::shared_ptr<ISessionStorage> storage);
    ~Client();

    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // ---- Storage & Config ----
    std::shared_ptr<ISessionStorage> storage() const;
    void setDefaultParseMode(ParseMode mode);
    ParseMode getDefaultParseMode() const;

    // ---- Thread Pool & Worker Concurrency ----
    void setThreadPoolSize(size_t threads);
    size_t getThreadPoolSize() const;

    // ---- Middleware Pipeline ----
    void use(MiddlewareFunc middleware);

    // ---- Plugin Architecture ----
    bool loadPlugin(std::shared_ptr<IPlugin> plugin);
    bool unloadPlugin(const std::string& name);
    std::vector<std::shared_ptr<IPlugin>> plugins() const;

    // ---- Auth ----
    void login(const std::string& phone,
               std::function<std::string()> code_callback = {},
               std::function<std::string()> password_callback = {});
    void loginBot(const std::string& token);
    void logout();
    AuthState authState() const;

    // ---- Users & chats ----
    User                 getMe();
    User                 getUser(UserId id);
    Chat                 getChat(ChatId id);
    std::vector<Message> getHistory(ChatId chat_id, int limit = 100);

    // ---- Message Threads & Topics ----
    MessageThreadInfo    getMessageThread(ChatId chat_id, MessageId message_id);
    std::vector<Message> getMessageThreadHistory(ChatId chat_id, MessageId message_id,
                                                 MessageId from_message_id = 0,
                                                 int offset = 0, int limit = 100);

    // ---- Secret Chats (E2EE) ----
    SecretChat           createSecretChat(UserId user_id);
    void                 closeSecretChat(int32_t secret_chat_id);
    SecretChat           getSecretChat(int32_t secret_chat_id);

    // ---- Business Bots & Connections ----
    BusinessConnection   getBusinessConnection(const std::string& business_connection_id);

    // ---- Voice & Group Calls ----
    GroupCall            getGroupCall(int32_t group_call_id);
    void                 joinGroupCall(int32_t group_call_id, const CallProtocol& protocol = {});
    void                 leaveGroupCall(int32_t group_call_id);

    // ---- Web Apps & Mini Apps ----
    void                 answerWebAppQuery(const std::string& web_app_query_id,
                                           const SentWebAppMessage& result);

    // ---- Text messaging ----
    Message sendMessage(ChatId chat_id, const std::string& text,
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendMessage(ChatId chat_id, const std::string& text,
                        const SendMessageOptions& options,
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendMessage(ChatId chat_id, const std::string& text,
                        ParseMode parse_mode,
                        const SendMessageOptions& options = {},
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendMessage(ChatId chat_id, const std::string& text,
                        const ReplyMarkup& markup,
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendMessage(ChatId chat_id, const std::string& text,
                        const ReplyMarkup& markup,
                        const SendMessageOptions& options,
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendMessage(ChatId chat_id, const std::string& text,
                        const ReplyMarkup& markup,
                        ParseMode parse_mode,
                        const SendMessageOptions& options = {},
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendFormattedMessage(ChatId chat_id, const FormattedText& text,
                                 std::optional<MessageId> reply_to = std::nullopt,
                                 const std::optional<ReplyMarkup>& markup = std::nullopt,
                                 const SendMessageOptions& options = {});
    void    editMessage(ChatId, MessageId, const std::string& text);
    void    editMessage(ChatId, MessageId, const std::string& text, ParseMode parse_mode);
    void    editMessageCaption(ChatId, MessageId, const std::string& caption);
    void    editMessageCaption(ChatId, MessageId, const std::string& caption, ParseMode parse_mode);
    void    editMessageReplyMarkup(ChatId, MessageId, const InlineKeyboard& markup);
    void    deleteMessages(ChatId, std::vector<MessageId>);
    void    setReaction(ChatId, MessageId, const std::string& emoji);
    Message forwardMessage(ChatId from_chat, MessageId msg_id, ChatId to_chat);
    void    pinMessage(ChatId, MessageId, bool disable_notification = false);
    void    unpinMessage(ChatId, MessageId);
    void    unpinAllMessages(ChatId);

    // ---- Scheduled messages ----
    Message sendScheduledMessage(ChatId chat_id, const std::string& text,
                                 Timestamp schedule_date,
                                 std::optional<MessageId> reply_to = std::nullopt);
    std::vector<Message> getScheduledMessages(ChatId chat_id);
    void sendScheduledMessageNow(ChatId chat_id, MessageId message_id);
    void deleteScheduledMessages(ChatId chat_id, std::vector<MessageId> message_ids);

    // ---- Media messaging ----
    Message sendPhoto(ChatId, const InputFile&,
                      const std::string& caption = "",
                      std::optional<MessageId> reply_to = std::nullopt);
    Message sendPhoto(ChatId, const InputFile&,
                      const std::string& caption,
                      ParseMode parse_mode,
                      const SendMessageOptions& options = {},
                      std::optional<MessageId> reply_to = std::nullopt);
    Message sendVideo(ChatId, const InputFile&,
                      const std::string& caption = "",
                      std::optional<MessageId> reply_to = std::nullopt,
                      int width = 0, int height = 0, int duration = 0);
    Message sendVideo(ChatId, const InputFile&,
                      const std::string& caption,
                      ParseMode parse_mode,
                      const SendMessageOptions& options = {},
                      std::optional<MessageId> reply_to = std::nullopt,
                      int width = 0, int height = 0, int duration = 0);
    Message sendDocument(ChatId, const InputFile&,
                         const std::string& caption = "",
                         std::optional<MessageId> reply_to = std::nullopt);
    Message sendDocument(ChatId, const InputFile&,
                         const std::string& caption,
                         ParseMode parse_mode,
                         const SendMessageOptions& options = {},
                         std::optional<MessageId> reply_to = std::nullopt);
    Message sendAudio(ChatId, const InputFile&,
                      const std::string& caption = "",
                      std::optional<MessageId> reply_to = std::nullopt,
                      int duration = 0,
                      const std::string& title = "",
                      const std::string& performer = "");
    Message sendAudio(ChatId, const InputFile&,
                      const std::string& caption,
                      ParseMode parse_mode,
                      const SendMessageOptions& options = {},
                      std::optional<MessageId> reply_to = std::nullopt,
                      int duration = 0,
                      const std::string& title = "",
                      const std::string& performer = "");
    Message sendVoiceNote(ChatId, const InputFile&,
                          const std::string& caption = "",
                          std::optional<MessageId> reply_to = std::nullopt,
                          int duration = 0);
    Message sendVoiceNote(ChatId, const InputFile&,
                          const std::string& caption,
                          ParseMode parse_mode,
                          const SendMessageOptions& options = {},
                          std::optional<MessageId> reply_to = std::nullopt,
                          int duration = 0);
    Message sendVideoNote(ChatId, const InputFile&,
                          std::optional<MessageId> reply_to = std::nullopt,
                          int duration = 0, int length = 0);
    Message sendVideoNote(ChatId, const InputFile&,
                          const SendMessageOptions& options,
                          std::optional<MessageId> reply_to = std::nullopt,
                          int duration = 0, int length = 0);
    Message sendAnimation(ChatId, const InputFile&,
                          const std::string& caption = "",
                          std::optional<MessageId> reply_to = std::nullopt,
                          int width = 0, int height = 0, int duration = 0);
    Message sendAnimation(ChatId, const InputFile&,
                          const std::string& caption,
                          ParseMode parse_mode,
                          const SendMessageOptions& options = {},
                          std::optional<MessageId> reply_to = std::nullopt,
                          int width = 0, int height = 0, int duration = 0);
    Message sendSticker(ChatId, const InputFile&,
                        std::optional<MessageId> reply_to = std::nullopt);
    Message sendSticker(ChatId, const InputFile&,
                        const SendMessageOptions& options,
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
    void setProfilePhoto(const InputFile& photo);
    void deleteProfilePhoto(std::int64_t profile_photo_id);
    UserProfilePhotos getUserProfilePhotos(UserId user_id, int offset = 0, int limit = 100);

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

    // ---- Async Coroutines (C++20) ----
    Task<Message>              asyncSendMessage(ChatId chat_id, const std::string& text,
                                                std::optional<MessageId> reply_to = std::nullopt);
    Task<Message>              asyncSendMessage(ChatId chat_id, const std::string& text,
                                                const SendMessageOptions& options,
                                                std::optional<MessageId> reply_to = std::nullopt);
    Task<Message>              asyncSendMessage(ChatId chat_id, const std::string& text,
                                                ParseMode parse_mode,
                                                const SendMessageOptions& options = {},
                                                std::optional<MessageId> reply_to = std::nullopt);
    Task<User>                 asyncGetMe();
    Task<User>                 asyncGetUser(UserId id);
    Task<Chat>                 asyncGetChat(ChatId id);
    Task<MessageThreadInfo>    asyncGetMessageThread(ChatId chat_id, MessageId message_id);
    Task<std::vector<Message>> asyncGetMessageThreadHistory(ChatId chat_id, MessageId message_id,
                                                           MessageId from_message_id = 0,
                                                           int offset = 0, int limit = 100);
    Task<SecretChat>           asyncCreateSecretChat(UserId user_id);
    Task<void>                 asyncCloseSecretChat(int32_t secret_chat_id);
    Task<SecretChat>           asyncGetSecretChat(int32_t secret_chat_id);
    Task<BusinessConnection>   asyncGetBusinessConnection(const std::string& business_connection_id);
    Task<GroupCall>            asyncGetGroupCall(int32_t group_call_id);
    Task<void>                 asyncJoinGroupCall(int32_t group_call_id, const CallProtocol& protocol = {});
    Task<void>                 asyncLeaveGroupCall(int32_t group_call_id);
    Task<void>                 asyncAnswerWebAppQuery(const std::string& web_app_query_id,
                                                      const SentWebAppMessage& result);
    Task<void>                 asyncDeleteMessages(ChatId chat_id, std::vector<MessageId> message_ids);
    Task<void>                 asyncEditMessage(ChatId chat_id, MessageId msg_id, const std::string& text);

    // ---- Event loop ----
    void run();
    void stop();

private:
    std::shared_ptr<ClientImpl> impl_;
};

} // namespace cppgram
