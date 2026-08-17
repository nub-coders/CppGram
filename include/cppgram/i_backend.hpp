#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include "cppgram/types.hpp"
#include "cppgram/keyboard.hpp"

namespace cppgram {
class Message;
class User;
class Chat;
struct ChatMember;
struct FileInfo;
struct PhotoSize;
struct UserProfilePhotos;
struct MessageThreadInfo;
struct SentWebAppMessage;
struct SecretChat;
struct BusinessConnection;
struct GroupCall;
struct CallProtocol;

class IBackend {
public:
    virtual ~IBackend() = default;

    // ---- Queries ----
    virtual User getMe() = 0;
    virtual User getUser(UserId id) = 0;
    virtual Chat getChat(ChatId id) = 0;
    virtual std::vector<Message> getHistory(ChatId chat_id, int limit) = 0;

    // ---- Thread & Topics ----
    virtual MessageThreadInfo getMessageThread(ChatId chat_id, MessageId message_id) = 0;
    virtual std::vector<Message> getMessageThreadHistory(ChatId chat_id, MessageId message_id,
                                                        MessageId from_message_id, int offset, int limit) = 0;

    // ---- Text messaging ----
    virtual Message sendMessage(ChatId chat_id, const std::string& text,
                                std::optional<MessageId> reply_to) = 0;
    virtual Message sendMessage(ChatId chat_id, const std::string& text,
                                std::optional<MessageId> reply_to,
                                ParseMode parse_mode,
                                const SendMessageOptions& options) = 0;
    virtual Message sendMessageWithMarkup(ChatId chat_id, const std::string& text,
                                          std::optional<MessageId> reply_to,
                                          const ReplyMarkup& markup) = 0;
    virtual Message sendMessageWithMarkup(ChatId chat_id, const std::string& text,
                                          std::optional<MessageId> reply_to,
                                          const ReplyMarkup& markup,
                                          ParseMode parse_mode,
                                          const SendMessageOptions& options) = 0;
    virtual Message sendFormattedMessage(ChatId chat_id, const FormattedText& text,
                                         std::optional<MessageId> reply_to,
                                         const std::optional<ReplyMarkup>& markup,
                                         const SendMessageOptions& options) = 0;
    virtual void    editMessage(ChatId, MessageId, const std::string& text) = 0;
    virtual void    editMessage(ChatId, MessageId, const std::string& text, ParseMode parse_mode) = 0;
    virtual void    editMessageCaption(ChatId, MessageId, const std::string& caption) = 0;
    virtual void    editMessageCaption(ChatId, MessageId, const std::string& caption, ParseMode parse_mode) = 0;
    virtual void    editMessageReplyMarkup(ChatId, MessageId, const InlineKeyboard& markup) = 0;
    virtual void    deleteMessages(ChatId, std::vector<MessageId>) = 0;
    virtual void    setReaction(ChatId, MessageId, const std::string& emoji) = 0;
    virtual Message forwardMessage(ChatId from_chat, MessageId msg_id, ChatId to_chat) = 0;
    virtual void    pinMessage(ChatId, MessageId, bool disable_notification) = 0;
    virtual void    unpinMessage(ChatId, MessageId) = 0;
    virtual void    unpinAllMessages(ChatId) = 0;

    // ---- Scheduled messages ----
    virtual std::vector<Message> getScheduledMessages(ChatId chat_id) = 0;
    virtual void sendScheduledMessageNow(ChatId chat_id, MessageId message_id) = 0;
    virtual void deleteScheduledMessages(ChatId chat_id, std::vector<MessageId> message_ids) = 0;

    // ---- Media messaging ----
    virtual Message sendPhoto(ChatId, const InputFile&, const std::string& caption,
                              std::optional<MessageId> reply_to) = 0;
    virtual Message sendPhoto(ChatId, const InputFile&, const std::string& caption,
                              std::optional<MessageId> reply_to,
                              ParseMode parse_mode,
                              const SendMessageOptions& options) = 0;
    virtual Message sendVideo(ChatId, const InputFile&, const std::string& caption,
                              std::optional<MessageId> reply_to,
                              int width, int height, int duration) = 0;
    virtual Message sendVideo(ChatId, const InputFile&, const std::string& caption,
                              std::optional<MessageId> reply_to,
                              ParseMode parse_mode,
                              const SendMessageOptions& options,
                              int width, int height, int duration) = 0;
    virtual Message sendDocument(ChatId, const InputFile&, const std::string& caption,
                                 std::optional<MessageId> reply_to) = 0;
    virtual Message sendDocument(ChatId, const InputFile&, const std::string& caption,
                                 std::optional<MessageId> reply_to,
                                 ParseMode parse_mode,
                                 const SendMessageOptions& options) = 0;
    virtual Message sendAudio(ChatId, const InputFile&, const std::string& caption,
                              std::optional<MessageId> reply_to, int duration,
                              const std::string& title, const std::string& performer) = 0;
    virtual Message sendAudio(ChatId, const InputFile&, const std::string& caption,
                              std::optional<MessageId> reply_to,
                              ParseMode parse_mode,
                              const SendMessageOptions& options,
                              int duration,
                              const std::string& title, const std::string& performer) = 0;
    virtual Message sendVoiceNote(ChatId, const InputFile&, const std::string& caption,
                                  std::optional<MessageId> reply_to, int duration) = 0;
    virtual Message sendVoiceNote(ChatId, const InputFile&, const std::string& caption,
                                  std::optional<MessageId> reply_to,
                                  ParseMode parse_mode,
                                  const SendMessageOptions& options,
                                  int duration) = 0;
    virtual Message sendVideoNote(ChatId, const InputFile&,
                                  std::optional<MessageId> reply_to,
                                  int duration, int length) = 0;
    virtual Message sendVideoNote(ChatId, const InputFile&,
                                  std::optional<MessageId> reply_to,
                                  const SendMessageOptions& options,
                                  int duration, int length) = 0;
    virtual Message sendAnimation(ChatId, const InputFile&, const std::string& caption,
                                  std::optional<MessageId> reply_to,
                                  int width, int height, int duration) = 0;
    virtual Message sendAnimation(ChatId, const InputFile&, const std::string& caption,
                                  std::optional<MessageId> reply_to,
                                  ParseMode parse_mode,
                                  const SendMessageOptions& options,
                                  int width, int height, int duration) = 0;
    virtual Message sendSticker(ChatId, const InputFile&,
                                std::optional<MessageId> reply_to) = 0;
    virtual Message sendSticker(ChatId, const InputFile&,
                                std::optional<MessageId> reply_to,
                                const SendMessageOptions& options) = 0;

    // ---- Rich messages ----
    virtual Message sendPoll(ChatId, const std::string& question,
                             std::vector<std::string> options, const PollConfig& config) = 0;
    virtual Message sendDice(ChatId, const std::string& emoji) = 0;
    virtual Message sendContact(ChatId, const Contact&) = 0;
    virtual Message sendLocation(ChatId, const Location&) = 0;
    virtual Message sendVenue(ChatId, const Venue&) = 0;
    virtual void    stopPoll(ChatId, MessageId) = 0;

    // ---- Chat management ----
    virtual void              leaveChat(ChatId) = 0;
    virtual void              joinChat(ChatId) = 0;
    virtual std::vector<User> getChatMembers(ChatId) = 0;
    virtual ChatId createGroup(const std::string& title, std::vector<UserId> members) = 0;
    virtual ChatId createSupergroup(const std::string& title, bool is_channel,
                                    const std::string& description) = 0;
    virtual void setChatTitle(ChatId, const std::string& title) = 0;
    virtual void setChatDescription(ChatId, const std::string& description) = 0;
    virtual void setChatPhoto(ChatId, const InputFile&) = 0;
    virtual void deleteChatPhoto(ChatId) = 0;
    virtual void setChatPermissions(ChatId, const ChatPermissions&) = 0;
    virtual void banChatMember(ChatId, UserId) = 0;
    virtual void unbanChatMember(ChatId, UserId) = 0;
    virtual void restrictChatMember(ChatId, UserId, const ChatPermissions&) = 0;
    virtual void promoteChatMember(ChatId, UserId, const ChatAdminRights&) = 0;
    virtual ChatMember getChatMember(ChatId, UserId) = 0;
    virtual int getChatMemberCount(ChatId) = 0;
    virtual std::vector<ChatMember> getChatAdministrators(ChatId) = 0;
    virtual std::string getChatInviteLink(ChatId) = 0;

    // ---- User operations ----
    virtual void blockUser(UserId) = 0;
    virtual void unblockUser(UserId) = 0;
    virtual void setProfilePhoto(const InputFile&) = 0;
    virtual void deleteProfilePhoto(std::int64_t profile_photo_id) = 0;
    virtual UserProfilePhotos getUserProfilePhotos(UserId, int offset, int limit) = 0;

    // ---- Text parsing helper ----
    virtual void setDefaultParseMode(ParseMode) = 0;
    virtual ParseMode getDefaultParseMode() const = 0;
    virtual FormattedText parseTextEntities(const std::string& text, ParseMode parse_mode) = 0;

    // ---- File operations ----
    virtual FileInfo getFile(FileId) = 0;
    virtual std::string downloadFile(FileId, const std::string& directory,
                                     std::function<void(FileProgress)> progress) = 0;

    // ---- Search ----
    virtual std::vector<Message> searchMessages(const std::string& query, int limit) = 0;
    virtual std::vector<Message> searchChatMessages(ChatId, const std::string& query,
                                                    int limit) = 0;

    // ---- Callback queries ----
    virtual void answerCallbackQuery(std::int64_t query_id, const std::string& text,
                                     bool show_alert, const std::string& url,
                                     int cache_time) = 0;

    // ---- Web Apps ----
    virtual void answerWebAppQuery(const std::string& web_app_query_id,
                                   const SentWebAppMessage& result) = 0;

    // ---- Secret Chats ----
    virtual SecretChat createSecretChat(UserId user_id) = 0;
    virtual void closeSecretChat(int32_t secret_chat_id) = 0;
    virtual SecretChat getSecretChat(int32_t secret_chat_id) = 0;

    // ---- Business Bots ----
    virtual BusinessConnection getBusinessConnection(const std::string& business_connection_id) = 0;

    // ---- Calls ----
    virtual GroupCall getGroupCall(int32_t group_call_id) = 0;
    virtual void joinGroupCall(int32_t group_call_id, const CallProtocol& protocol) = 0;
    virtual void leaveGroupCall(int32_t group_call_id) = 0;
};

} // namespace cppgram
