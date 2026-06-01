#pragma once
#include <string>
#include <memory>
#include <optional>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"
#include "cppgram/media.hpp"
#include "cppgram/keyboard.hpp"

namespace cppgram {
class IBackend;

class Message {
public:
    MessageId   id{};
    ChatId      chat_id{};
    ChatType    chat_type{ChatType::Unknown};
    std::string text;
    std::string caption;
    User        sender;
    Timestamp   date{};
    bool        outgoing{false};
    MediaType   media{MediaType::None};
    std::optional<MessageId> reply_to;
    std::optional<ForwardInfo> forward_info;

    // Media attachments
    std::optional<Photo>          photo;
    std::optional<VideoInfo>      video;
    std::optional<DocumentInfo>   document;
    std::optional<AudioInfo>      audio;
    std::optional<VoiceNoteInfo>  voice_note;
    std::optional<VideoNoteInfo>  video_note;
    std::optional<AnimationInfo>  animation;
    std::optional<StickerInfo>    sticker;

    // Rich content
    std::optional<Poll>     poll;
    std::optional<Location> location;
    std::optional<Contact>  contact;
    std::optional<Venue>    venue;
    std::optional<Dice>     dice;

    // Reply markup
    std::optional<InlineKeyboard> reply_markup;

    // ---- Methods ----
    Message reply(const std::string& body);
    Message reply(const std::string& body, const ReplyMarkup& markup);
    void    edit(const std::string& body);
    void    editCaption(const std::string& new_caption);
    void    editReplyMarkup(const InlineKeyboard& markup);
    void    deleteMessage();
    void    react(const std::string& emoji);
    Message forward(ChatId target_chat);
    void    pin(bool disable_notification = false);
    void    unpin();

    bool has_media() const noexcept { return media != MediaType::None; }
    bool is_text() const noexcept { return !text.empty() && media == MediaType::None; }
    bool is_command() const noexcept { return !text.empty() && text[0] == '/'; }

    std::weak_ptr<IBackend> _client;
};

} // namespace cppgram
