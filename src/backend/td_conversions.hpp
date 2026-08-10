#pragma once

/**
 * @file td_conversions.hpp
 * @brief Type conversion and serialization layer between TDLib and CppGram domain models.
 * 
 * Provides bidirectional conversion helpers for:
 * - Enumerations (ChatType, MediaType, MessageEntityType, ParseMode)
 * - Media types (Photo, Video, Audio, Document, Voice/Video notes, Stickers, Polls)
 * - Keyboards and interactive reply markups
 * - Core entities (User, Chat, Message, CallbackQuery)
 * - Text formatting and TDLib entity serialization
 */

#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"
#include "cppgram/chat.hpp"
#include "cppgram/message.hpp"
#include "cppgram/media.hpp"
#include "cppgram/keyboard.hpp"
#include "cppgram/callback_query.hpp"
#include "cppgram/i_backend.hpp"

namespace cppgram::detail {

namespace td_api = td::td_api;

// Forward declaration
inline std::vector<MessageEntity> convert_entities(
    const std::vector<td_api::object_ptr<td_api::textEntity>>& entities);

// ============================================================================
// Type and Enumeration Conversions
// ============================================================================

/**
 * @brief Maps a TDLib ChatType object to the CppGram ChatType enum.
 * @param ct The TDLib ChatType object.
 * @return Corresponding CppGram ChatType.
 */
inline ChatType convert_chat_type(const td_api::ChatType& ct) {
    switch (ct.get_id()) {
        case td_api::chatTypePrivate::ID:    return ChatType::Private;
        case td_api::chatTypeBasicGroup::ID: return ChatType::BasicGroup;
        case td_api::chatTypeSupergroup::ID: {
            auto& sg = static_cast<const td_api::chatTypeSupergroup&>(ct);
            return sg.is_channel_ ? ChatType::Channel : ChatType::Supergroup;
        }
        case td_api::chatTypeSecret::ID:     return ChatType::Secret;
        default:                             return ChatType::Unknown;
    }
}

/**
 * @brief Determines the CppGram MediaType from a TDLib MessageContent object.
 * @param mc The TDLib MessageContent object.
 * @return Detected MediaType classification.
 */
inline MediaType classify_media(const td_api::MessageContent& mc) {
    switch (mc.get_id()) {
        case td_api::messagePhoto::ID:       return MediaType::Photo;
        case td_api::messageVideo::ID:       return MediaType::Video;
        case td_api::messageDocument::ID:    return MediaType::Document;
        case td_api::messageAudio::ID:       return MediaType::Audio;
        case td_api::messageVoiceNote::ID:   return MediaType::Voice;
        case td_api::messageVideoNote::ID:   return MediaType::VideoNote;
        case td_api::messageAnimation::ID:   return MediaType::Animation;
        case td_api::messageSticker::ID:     return MediaType::Sticker;
        case td_api::messagePoll::ID:        return MediaType::Poll;
        case td_api::messageLocation::ID:    return MediaType::Location;
        case td_api::messageContact::ID:     return MediaType::Contact;
        case td_api::messageVenue::ID:       return MediaType::Venue;
        case td_api::messageDice::ID:        return MediaType::Dice;
        default:                             return MediaType::None;
    }
}

// ============================================================================
// Photo and Thumbnail Conversions
// ============================================================================

/**
 * @brief Converts a TDLib photoSize object to a PhotoSize domain model.
 * @param ps The TDLib photoSize instance.
 * @return Converted PhotoSize domain model.
 */
inline PhotoSize convert_photo_size(const td_api::photoSize& ps) {
    PhotoSize out;
    out.width = ps.width_;
    out.height = ps.height_;
    if (ps.photo_) {
        out.file_id = ps.photo_->id_;
        out.file_size = ps.photo_->size_;
        if (ps.photo_->remote_)
            out.file_unique_id = ps.photo_->remote_->unique_id_;
    }
    return out;
}

/**
 * @brief Converts an optional TDLib thumbnail pointer to a PhotoSize domain model.
 * @param t Pointer to TDLib thumbnail (may be null).
 * @return PhotoSize if present and valid, std::nullopt otherwise.
 */
inline std::optional<PhotoSize> convert_thumbnail(const td_api::thumbnail* t) {
    if (!t || !t->file_) return std::nullopt;
    PhotoSize ps;
    ps.width = t->width_;
    ps.height = t->height_;
    ps.file_id = t->file_->id_;
    ps.file_size = t->file_->size_;
    if (t->file_->remote_)
        ps.file_unique_id = t->file_->remote_->unique_id_;
    return ps;
}

// ============================================================================
// Media Extraction from Message Content
// ============================================================================

/**
 * @brief Extracts specific media payloads and captions from TDLib MessageContent into a Message.
 * @param mc The TDLib message content object.
 * @param out Target Message domain entity to populate.
 */
inline void extract_media(const td_api::MessageContent& mc, Message& out) {
    switch (mc.get_id()) {
    case td_api::messagePhoto::ID: {
        auto& mp = static_cast<const td_api::messagePhoto&>(mc);
        if (mp.photo_) {
            Photo photo;
            for (auto& s : mp.photo_->sizes_)
                if (s) photo.sizes.push_back(convert_photo_size(*s));
            if (!photo.empty()) out.photo = std::move(photo);
        }
        if (mp.caption_) out.caption = mp.caption_->text_;
        break;
    }
    case td_api::messageVideo::ID: {
        auto& mv = static_cast<const td_api::messageVideo&>(mc);
        if (mv.video_) {
            VideoInfo vi;
            vi.width = mv.video_->width_;
            vi.height = mv.video_->height_;
            vi.duration = mv.video_->duration_;
            vi.mime_type = mv.video_->mime_type_;
            vi.file_name = mv.video_->file_name_;
            if (mv.video_->video_) {
                vi.file_id = mv.video_->video_->id_;
                vi.file_size = mv.video_->video_->size_;
                if (mv.video_->video_->remote_)
                    vi.file_unique_id = mv.video_->video_->remote_->unique_id_;
            }
            vi.thumbnail = convert_thumbnail(mv.video_->thumbnail_.get());
            out.video = std::move(vi);
        }
        if (mv.caption_) out.caption = mv.caption_->text_;
        break;
    }
    case td_api::messageDocument::ID: {
        auto& md = static_cast<const td_api::messageDocument&>(mc);
        if (md.document_) {
            DocumentInfo di;
            di.file_name = md.document_->file_name_;
            di.mime_type = md.document_->mime_type_;
            if (md.document_->document_) {
                di.file_id = md.document_->document_->id_;
                di.file_size = md.document_->document_->size_;
                if (md.document_->document_->remote_)
                    di.file_unique_id = md.document_->document_->remote_->unique_id_;
            }
            di.thumbnail = convert_thumbnail(md.document_->thumbnail_.get());
            out.document = std::move(di);
        }
        if (md.caption_) out.caption = md.caption_->text_;
        break;
    }
    case td_api::messageAudio::ID: {
        auto& ma = static_cast<const td_api::messageAudio&>(mc);
        if (ma.audio_) {
            AudioInfo ai;
            ai.duration = ma.audio_->duration_;
            ai.title = ma.audio_->title_;
            ai.performer = ma.audio_->performer_;
            ai.mime_type = ma.audio_->mime_type_;
            if (ma.audio_->audio_) {
                ai.file_id = ma.audio_->audio_->id_;
                ai.file_size = ma.audio_->audio_->size_;
                if (ma.audio_->audio_->remote_)
                    ai.file_unique_id = ma.audio_->audio_->remote_->unique_id_;
            }
            ai.thumbnail = convert_thumbnail(ma.audio_->album_cover_thumbnail_.get());
            out.audio = std::move(ai);
        }
        if (ma.caption_) out.caption = ma.caption_->text_;
        break;
    }
    case td_api::messageVoiceNote::ID: {
        auto& mvn = static_cast<const td_api::messageVoiceNote&>(mc);
        if (mvn.voice_note_) {
            VoiceNoteInfo vni;
            vni.duration = mvn.voice_note_->duration_;
            vni.mime_type = mvn.voice_note_->mime_type_;
            if (mvn.voice_note_->voice_) {
                vni.file_id = mvn.voice_note_->voice_->id_;
                vni.file_size = mvn.voice_note_->voice_->size_;
                if (mvn.voice_note_->voice_->remote_)
                    vni.file_unique_id = mvn.voice_note_->voice_->remote_->unique_id_;
            }
            out.voice_note = std::move(vni);
        }
        if (mvn.caption_) out.caption = mvn.caption_->text_;
        break;
    }
    case td_api::messageVideoNote::ID: {
        auto& mvn = static_cast<const td_api::messageVideoNote&>(mc);
        if (mvn.video_note_) {
            VideoNoteInfo vni;
            vni.length = mvn.video_note_->length_;
            vni.duration = mvn.video_note_->duration_;
            if (mvn.video_note_->video_) {
                vni.file_id = mvn.video_note_->video_->id_;
                vni.file_size = mvn.video_note_->video_->size_;
                if (mvn.video_note_->video_->remote_)
                    vni.file_unique_id = mvn.video_note_->video_->remote_->unique_id_;
            }
            vni.thumbnail = convert_thumbnail(mvn.video_note_->thumbnail_.get());
            out.video_note = std::move(vni);
        }
        break;
    }
    case td_api::messageAnimation::ID: {
        auto& ma = static_cast<const td_api::messageAnimation&>(mc);
        if (ma.animation_) {
            AnimationInfo ai;
            ai.width = ma.animation_->width_;
            ai.height = ma.animation_->height_;
            ai.duration = ma.animation_->duration_;
            ai.mime_type = ma.animation_->mime_type_;
            ai.file_name = ma.animation_->file_name_;
            if (ma.animation_->animation_) {
                ai.file_id = ma.animation_->animation_->id_;
                ai.file_size = ma.animation_->animation_->size_;
                if (ma.animation_->animation_->remote_)
                    ai.file_unique_id = ma.animation_->animation_->remote_->unique_id_;
            }
            ai.thumbnail = convert_thumbnail(ma.animation_->thumbnail_.get());
            out.animation = std::move(ai);
        }
        if (ma.caption_) out.caption = ma.caption_->text_;
        break;
    }
    case td_api::messageSticker::ID: {
        auto& ms = static_cast<const td_api::messageSticker&>(mc);
        if (ms.sticker_) {
            StickerInfo si;
            si.width = ms.sticker_->width_;
            si.height = ms.sticker_->height_;
            si.emoji = ms.sticker_->emoji_;
            si.is_animated = ms.sticker_->is_animated_;
            si.is_video = false;
            if (ms.sticker_->sticker_) {
                si.file_id = ms.sticker_->sticker_->id_;
                si.file_size = ms.sticker_->sticker_->size_;
                if (ms.sticker_->sticker_->remote_)
                    si.file_unique_id = ms.sticker_->sticker_->remote_->unique_id_;
            }
            si.thumbnail = convert_thumbnail(ms.sticker_->thumbnail_.get());
            out.sticker = std::move(si);
        }
        break;
    }
    case td_api::messagePoll::ID: {
        auto& mp = static_cast<const td_api::messagePoll&>(mc);
        if (mp.poll_) {
            Poll p;
            p.id = mp.poll_->id_;
            p.question = mp.poll_->question_;
            for (auto& o : mp.poll_->options_) {
                if (o) {
                    PollOption po;
                    po.text = o->text_;
                    po.voter_count = o->voter_count_;
                    p.options.push_back(std::move(po));
                }
            }
            p.total_voter_count = mp.poll_->total_voter_count_;
            p.is_closed = mp.poll_->is_closed_;
            p.is_anonymous = mp.poll_->is_anonymous_;
            if (mp.poll_->type_) {
                if (mp.poll_->type_->get_id() == td_api::pollTypeQuiz::ID) {
                    p.type = PollType::Quiz;
                    auto& qt = static_cast<const td_api::pollTypeQuiz&>(*mp.poll_->type_);
                    p.correct_option_id = qt.correct_option_id_;
                    if (qt.explanation_) p.explanation = qt.explanation_->text_;
                } else {
                    p.type = PollType::Regular;
                    auto& rt = static_cast<const td_api::pollTypeRegular&>(*mp.poll_->type_);
                    p.allows_multiple_answers = rt.allow_multiple_answers_;
                }
            }
            out.poll = std::move(p);
        }
        break;
    }
    case td_api::messageLocation::ID: {
        auto& ml = static_cast<const td_api::messageLocation&>(mc);
        if (ml.location_) {
            Location loc;
            loc.latitude = ml.location_->latitude_;
            loc.longitude = ml.location_->longitude_;
            loc.horizontal_accuracy = ml.location_->horizontal_accuracy_;
            loc.live_period = ml.live_period_;
            loc.heading = ml.heading_;
            loc.proximity_alert_radius = ml.proximity_alert_radius_;
            out.location = loc;
        }
        break;
    }
    case td_api::messageContact::ID: {
        auto& mc2 = static_cast<const td_api::messageContact&>(mc);
        if (mc2.contact_) {
            Contact c;
            c.phone_number = mc2.contact_->phone_number_;
            c.first_name = mc2.contact_->first_name_;
            c.last_name = mc2.contact_->last_name_;
            c.user_id = mc2.contact_->user_id_;
            c.vcard = mc2.contact_->vcard_;
            out.contact = c;
        }
        break;
    }
    case td_api::messageVenue::ID: {
        auto& mv = static_cast<const td_api::messageVenue&>(mc);
        if (mv.venue_) {
            Venue v;
            if (mv.venue_->location_) {
                v.location.latitude = mv.venue_->location_->latitude_;
                v.location.longitude = mv.venue_->location_->longitude_;
                v.location.horizontal_accuracy = mv.venue_->location_->horizontal_accuracy_;
            }
            v.title = mv.venue_->title_;
            v.address = mv.venue_->address_;
            v.provider = mv.venue_->provider_;
            v.id = mv.venue_->id_;
            v.type = mv.venue_->type_;
            out.venue = v;
        }
        break;
    }
    case td_api::messageDice::ID: {
        auto& md = static_cast<const td_api::messageDice&>(mc);
        Dice d;
        d.emoji = md.emoji_;
        d.value = md.value_;
        out.dice = d;
        break;
    }
    default:
        break;
    }
}

// ============================================================================
// Keyboard and Reply Markup Conversions
// ============================================================================

/**
 * @brief Converts a TDLib ReplyMarkup into a CppGram InlineKeyboard (if applicable).
 * @param rm Pointer to the TDLib reply markup object.
 * @return InlineKeyboard if the markup is an inline keyboard, std::nullopt otherwise.
 */
inline std::optional<InlineKeyboard> convert_reply_markup(
        const td_api::ReplyMarkup* rm) {
    if (!rm) return std::nullopt;
    if (rm->get_id() != td_api::replyMarkupInlineKeyboard::ID) return std::nullopt;
    auto& ik = static_cast<const td_api::replyMarkupInlineKeyboard&>(*rm);
    InlineKeyboard out;
    for (auto& row : ik.rows_) {
        InlineKeyboardRow r;
        for (auto& btn : row) {
            if (!btn) continue;
            InlineKeyboardButton b;
            b.text = btn->text_;
            if (btn->type_) {
                switch (btn->type_->get_id()) {
                case td_api::inlineKeyboardButtonTypeUrl::ID:
                    b.url = static_cast<const td_api::inlineKeyboardButtonTypeUrl&>(
                        *btn->type_).url_;
                    break;
                case td_api::inlineKeyboardButtonTypeCallback::ID:
                    b.callback_data = static_cast<
                        const td_api::inlineKeyboardButtonTypeCallback&>(
                        *btn->type_).data_;
                    break;
                case td_api::inlineKeyboardButtonTypeSwitchInline::ID: {
                    auto& si = static_cast<
                        const td_api::inlineKeyboardButtonTypeSwitchInline&>(
                        *btn->type_);
                    b.switch_inline_query = si.query_;
                    break;
                }
                default: break;
                }
            }
            r.push_back(std::move(b));
        }
        out.rows.push_back(std::move(r));
    }
    return out;
}

/**
 * @brief Serializes a CppGram ReplyMarkup variant into the appropriate TDLib ReplyMarkup object.
 * @param rm The CppGram reply markup variant (InlineKeyboard, ReplyKeyboard, RemoveKeyboard, ForceReply).
 * @return TDLib object pointer to the corresponding ReplyMarkup, or nullptr if none.
 */
inline td_api::object_ptr<td_api::ReplyMarkup> build_reply_markup(
        const ReplyMarkup& rm) {
    if (auto* ik = std::get_if<InlineKeyboard>(&rm)) {
        auto markup = td_api::make_object<td_api::replyMarkupInlineKeyboard>();
        for (auto& row : ik->rows) {
            std::vector<td_api::object_ptr<td_api::inlineKeyboardButton>> td_row;
            for (auto& btn : row) {
                auto td_btn = td_api::make_object<td_api::inlineKeyboardButton>();
                td_btn->text_ = btn.text;
                if (!btn.callback_data.empty()) {
                    td_btn->type_ = td_api::make_object<
                        td_api::inlineKeyboardButtonTypeCallback>(btn.callback_data);
                } else if (!btn.url.empty()) {
                    td_btn->type_ = td_api::make_object<
                        td_api::inlineKeyboardButtonTypeUrl>(btn.url);
                } else if (!btn.switch_inline_query.empty()) {
                    td_btn->type_ = td_api::make_object<
                        td_api::inlineKeyboardButtonTypeSwitchInline>(
                            btn.switch_inline_query, false);
                } else if (!btn.switch_inline_query_current_chat.empty()) {
                    td_btn->type_ = td_api::make_object<
                        td_api::inlineKeyboardButtonTypeSwitchInline>(
                            btn.switch_inline_query_current_chat, true);
                }
                td_row.push_back(std::move(td_btn));
            }
            markup->rows_.push_back(std::move(td_row));
        }
        return markup;
    }
    if (auto* rk = std::get_if<ReplyKeyboard>(&rm)) {
        auto markup = td_api::make_object<td_api::replyMarkupShowKeyboard>();
        markup->resize_keyboard_ = rk->resize_keyboard;
        markup->one_time_ = rk->one_time_keyboard;
        markup->input_field_placeholder_ = rk->input_field_placeholder;
        for (auto& row : rk->rows) {
            std::vector<td_api::object_ptr<td_api::keyboardButton>> td_row;
            for (auto& btn : row) {
                auto td_btn = td_api::make_object<td_api::keyboardButton>();
                td_btn->text_ = btn.text;
                if (btn.request_contact)
                    td_btn->type_ = td_api::make_object<
                        td_api::keyboardButtonTypeRequestPhoneNumber>();
                else if (btn.request_location)
                    td_btn->type_ = td_api::make_object<
                        td_api::keyboardButtonTypeRequestLocation>();
                else
                    td_btn->type_ = td_api::make_object<
                        td_api::keyboardButtonTypeText>();
                td_row.push_back(std::move(td_btn));
            }
            markup->rows_.push_back(std::move(td_row));
        }
        return markup;
    }
    if (std::get_if<RemoveKeyboard>(&rm)) {
        return td_api::make_object<td_api::replyMarkupRemoveKeyboard>();
    }
    if (auto* fr = std::get_if<ForceReply>(&rm)) {
        auto markup = td_api::make_object<td_api::replyMarkupForceReply>();
        markup->input_field_placeholder_ = fr->input_field_placeholder;
        return markup;
    }
    return nullptr;
}

/**
 * @brief Helper to convert a typed InlineKeyboard into a TDLib ReplyMarkup.
 * @param ik The inline keyboard structure.
 * @return TDLib object pointer to the reply markup.
 */
inline td_api::object_ptr<td_api::ReplyMarkup> build_inline_keyboard(
        const InlineKeyboard& ik) {
    ReplyMarkup rm = ik;
    return build_reply_markup(rm);
}

// ============================================================================
// User and Chat Model Conversions
// ============================================================================

/**
 * @brief Converts a TDLib user object into a CppGram User domain entity.
 * @param u The TDLib user object.
 * @param backend Weak pointer to the backend client implementation.
 * @return Converted User instance.
 */
inline User convert_user(const td_api::user& u,
                         std::weak_ptr<IBackend> backend = {}) {
    User out;
    out.id           = u.id_;
    out.first_name   = u.first_name_;
    out.last_name    = u.last_name_;
    out.username     = u.username_;
    out.phone_number = u.phone_number_;
    out.is_bot       = u.type_ && u.type_->get_id() == td_api::userTypeBot::ID;
    out.is_premium   = false;
    out.is_verified  = u.is_verified_;
    out.is_scam      = u.is_scam_;
    out.is_fake      = u.is_fake_;
    out._client      = backend;
    return out;
}

/**
 * @brief Converts a TDLib chat object into a CppGram Chat domain entity.
 * @param c The TDLib chat object.
 * @param backend Weak pointer to the backend client implementation.
 * @return Converted Chat instance.
 */
inline Chat convert_chat(const td_api::chat& c,
                         std::weak_ptr<IBackend> backend = {}) {
    Chat out;
    out.id    = c.id_;
    out.title = c.title_;
    if (c.type_)
        out.type = convert_chat_type(*c.type_);
    out.has_protected_content = c.has_protected_content_;
    if (c.permissions_) {
        out.permissions.can_send_messages = c.permissions_->can_send_messages_;
        out.permissions.can_send_media = c.permissions_->can_send_media_messages_;
        out.permissions.can_send_polls = c.permissions_->can_send_polls_;
        out.permissions.can_send_other = c.permissions_->can_send_other_messages_;
        out.permissions.can_add_web_page_previews = c.permissions_->can_add_web_page_previews_;
        out.permissions.can_change_info = c.permissions_->can_change_info_;
        out.permissions.can_invite_users = c.permissions_->can_invite_users_;
        out.permissions.can_pin_messages = c.permissions_->can_pin_messages_;
        out.permissions.can_manage_topics = false;
    }
    out._client = backend;
    return out;
}

/**
 * @brief Maps a TDLib ChatMemberStatus object to the CppGram ChatMemberStatus enum.
 * @param s The TDLib chat member status object.
 * @return Corresponding ChatMemberStatus enum value.
 */
inline ChatMemberStatus convert_member_status(const td_api::ChatMemberStatus& s) {
    switch (s.get_id()) {
        case td_api::chatMemberStatusCreator::ID:       return ChatMemberStatus::Creator;
        case td_api::chatMemberStatusAdministrator::ID:  return ChatMemberStatus::Administrator;
        case td_api::chatMemberStatusMember::ID:         return ChatMemberStatus::Member;
        case td_api::chatMemberStatusRestricted::ID:     return ChatMemberStatus::Restricted;
        case td_api::chatMemberStatusLeft::ID:           return ChatMemberStatus::Left;
        case td_api::chatMemberStatusBanned::ID:         return ChatMemberStatus::Banned;
        default:                                         return ChatMemberStatus::Member;
    }
}

/**
 * @brief Constructs a TDLib chatPermissions object from CppGram ChatPermissions settings.
 * @param p CppGram ChatPermissions configuration.
 * @return TDLib chatPermissions object pointer.
 */
inline td_api::object_ptr<td_api::chatPermissions> build_chat_permissions(
        const ChatPermissions& p) {
    auto out = td_api::make_object<td_api::chatPermissions>();
    out->can_send_messages_ = p.can_send_messages;
    out->can_send_media_messages_ = p.can_send_media;
    out->can_send_polls_ = p.can_send_polls;
    out->can_send_other_messages_ = p.can_send_other;
    out->can_add_web_page_previews_ = p.can_add_web_page_previews;
    out->can_change_info_ = p.can_change_info;
    out->can_invite_users_ = p.can_invite_users;
    out->can_pin_messages_ = p.can_pin_messages;
    return out;
}

/**
 * @brief Converts a TDLib file object into a CppGram FileInfo descriptor.
 * @param f The TDLib file object.
 * @return Converted FileInfo instance.
 */
inline FileInfo convert_file(const td_api::file& f) {
    FileInfo out;
    out.file_id = f.id_;
    out.size = f.size_;
    out.expected_size = f.expected_size_;
    if (f.local_) {
        out.local_path = f.local_->path_;
        out.is_downloading = f.local_->is_downloading_active_;
        out.is_downloaded = f.local_->is_downloading_completed_;
        if (f.local_->downloaded_size_ > 0 && out.expected_size > 0)
            out.download_progress = static_cast<double>(f.local_->downloaded_size_)
                                    / out.expected_size;
    }
    if (f.remote_) {
        out.remote_file_id = f.remote_->id_;
        out.file_unique_id = f.remote_->unique_id_;
    }
    return out;
}

// ============================================================================
// Message and Callback Query Conversions
// ============================================================================

/**
 * @brief Converts a TDLib message into a full CppGram Message domain object.
 * @param m The TDLib message object.
 * @param chat_type Resolved chat type context.
 * @param backend Weak pointer to the backend client implementation.
 * @return Converted Message instance.
 */
inline Message convert_message(const td_api::message& m,
                               ChatType chat_type = ChatType::Unknown,
                               std::weak_ptr<IBackend> backend = {}) {
    Message out;
    out.id        = m.id_;
    out.chat_id   = m.chat_id_;
    out.chat_type = chat_type;
    out.outgoing  = m.is_outgoing_;
    out.date      = std::chrono::system_clock::from_time_t(m.date_);

    if (m.scheduling_state_ && m.scheduling_state_->get_id() == td_api::messageSchedulingStateSendAtDate::ID) {
        auto& ss = static_cast<const td_api::messageSchedulingStateSendAtDate&>(*m.scheduling_state_);
        out.schedule_date = std::chrono::system_clock::from_time_t(ss.send_date_);
    }

    // Extract text / caption
    if (m.content_) {
        if (m.content_->get_id() == td_api::messageText::ID) {
            auto& mt = static_cast<const td_api::messageText&>(*m.content_);
            if (mt.text_) {
                out.text = mt.text_->text_;
                out.entities = convert_entities(mt.text_->entities_);
            }
        }
        // Media type + details
        out.media = classify_media(*m.content_);
        extract_media(*m.content_, out);
    }

    // Reply markup
    out.reply_markup = convert_reply_markup(m.reply_markup_.get());

    // Reply
    if (m.reply_to_message_id_ != 0) {
        out.reply_to = m.reply_to_message_id_;
    }

    // Sender
    if (m.sender_id_ && m.sender_id_->get_id() == td_api::messageSenderUser::ID) {
        auto& su = static_cast<const td_api::messageSenderUser&>(*m.sender_id_);
        out.sender.id = su.user_id_;
    }

    // Forward info
    if (m.forward_info_ && m.forward_info_->origin_) {
        ForwardInfo fi;
        fi.origin_date = std::chrono::system_clock::from_time_t(m.forward_info_->date_);
        auto& origin = m.forward_info_->origin_;
        switch (origin->get_id()) {
            case td_api::messageForwardOriginUser::ID: {
                auto& o = static_cast<const td_api::messageForwardOriginUser&>(*origin);
                fi.origin_sender_id = o.sender_user_id_;
                break;
            }
            case td_api::messageForwardOriginChat::ID: {
                auto& o = static_cast<const td_api::messageForwardOriginChat&>(*origin);
                fi.origin_chat_id = o.sender_chat_id_;
                fi.origin_sender_name = o.author_signature_;
                break;
            }
            case td_api::messageForwardOriginChannel::ID: {
                auto& o = static_cast<const td_api::messageForwardOriginChannel&>(*origin);
                fi.origin_chat_id    = o.chat_id_;
                fi.origin_message_id = o.message_id_;
                fi.origin_sender_name = o.author_signature_;
                break;
            }
            case td_api::messageForwardOriginHiddenUser::ID: {
                auto& o = static_cast<const td_api::messageForwardOriginHiddenUser&>(*origin);
                fi.origin_sender_name = o.sender_name_;
                break;
            }
            default: break;
        }
        out.forward_info = std::move(fi);
    }

    out._client = backend;
    return out;
}

/**
 * @brief Converts an incoming TDLib callback query update into a CppGram CallbackQuery.
 * @param q The TDLib updateNewCallbackQuery object.
 * @param backend Weak pointer to the backend client implementation.
 * @return Converted CallbackQuery instance.
 */
inline CallbackQuery convert_callback_query(
        const td_api::updateNewCallbackQuery& q,
        std::weak_ptr<IBackend> backend = {}) {
    CallbackQuery out;
    out.id = q.id_;
    out.sender.id = q.sender_user_id_;
    out.chat_id = q.chat_id_;
    out.message_id = q.message_id_;
    out.chat_instance = std::to_string(q.chat_instance_);
    if (q.payload_ && q.payload_->get_id() == td_api::callbackQueryPayloadData::ID) {
        auto& d = static_cast<const td_api::callbackQueryPayloadData&>(*q.payload_);
        out.data = d.data_;
    }
    out._client = backend;
    return out;
}

// ============================================================================
// Text Entity and Formatting Conversions
// ============================================================================

/**
 * @brief Maps a TDLib TextEntityType to the CppGram MessageEntityType enum.
 * @param et The TDLib text entity type.
 * @return Corresponding MessageEntityType enum value.
 */
inline MessageEntityType convert_entity_type(const td_api::TextEntityType& et) {
    switch (et.get_id()) {
        case td_api::textEntityTypeMention::ID:       return MessageEntityType::Mention;
        case td_api::textEntityTypeHashtag::ID:       return MessageEntityType::Hashtag;
        case td_api::textEntityTypeCashtag::ID:       return MessageEntityType::Cashtag;
        case td_api::textEntityTypeBotCommand::ID:    return MessageEntityType::BotCommand;
        case td_api::textEntityTypeUrl::ID:           return MessageEntityType::Url;
        case td_api::textEntityTypeEmailAddress::ID:  return MessageEntityType::EmailAddress;
        case td_api::textEntityTypePhoneNumber::ID:   return MessageEntityType::PhoneNumber;
        case td_api::textEntityTypeBold::ID:          return MessageEntityType::Bold;
        case td_api::textEntityTypeItalic::ID:        return MessageEntityType::Italic;
        case td_api::textEntityTypeUnderline::ID:     return MessageEntityType::Underline;
        case td_api::textEntityTypeStrikethrough::ID: return MessageEntityType::Strikethrough;
        case td_api::textEntityTypeCode::ID:          return MessageEntityType::Code;
        case td_api::textEntityTypePre::ID:           return MessageEntityType::Pre;
        case td_api::textEntityTypePreCode::ID:       return MessageEntityType::PreCode;
        case td_api::textEntityTypeTextUrl::ID:       return MessageEntityType::TextUrl;
        case td_api::textEntityTypeMentionName::ID:   return MessageEntityType::MentionName;
        default:                                      return MessageEntityType::Unknown;
    }
}

/**
 * @brief Constructs a TDLib TextEntityType object from a CppGram MessageEntity.
 * @param ent The CppGram message entity.
 * @return TDLib object pointer to the corresponding TextEntityType, or nullptr if unmapped.
 */
inline td_api::object_ptr<td_api::TextEntityType> make_td_entity_type(const MessageEntity& ent) {
    switch (ent.type) {
        case MessageEntityType::Mention:       return td_api::make_object<td_api::textEntityTypeMention>();
        case MessageEntityType::Hashtag:       return td_api::make_object<td_api::textEntityTypeHashtag>();
        case MessageEntityType::Cashtag:       return td_api::make_object<td_api::textEntityTypeCashtag>();
        case MessageEntityType::BotCommand:    return td_api::make_object<td_api::textEntityTypeBotCommand>();
        case MessageEntityType::Url:           return td_api::make_object<td_api::textEntityTypeUrl>();
        case MessageEntityType::EmailAddress:  return td_api::make_object<td_api::textEntityTypeEmailAddress>();
        case MessageEntityType::PhoneNumber:   return td_api::make_object<td_api::textEntityTypePhoneNumber>();
        case MessageEntityType::Bold:          return td_api::make_object<td_api::textEntityTypeBold>();
        case MessageEntityType::Italic:        return td_api::make_object<td_api::textEntityTypeItalic>();
        case MessageEntityType::Underline:     return td_api::make_object<td_api::textEntityTypeUnderline>();
        case MessageEntityType::Strikethrough: return td_api::make_object<td_api::textEntityTypeStrikethrough>();
        case MessageEntityType::Code:          return td_api::make_object<td_api::textEntityTypeCode>();
        case MessageEntityType::Pre:           return td_api::make_object<td_api::textEntityTypePre>();
        case MessageEntityType::PreCode:       return td_api::make_object<td_api::textEntityTypePreCode>(ent.argument);
        case MessageEntityType::TextUrl:       return td_api::make_object<td_api::textEntityTypeTextUrl>(ent.argument);
        case MessageEntityType::MentionName:   return td_api::make_object<td_api::textEntityTypeMentionName>(ent.argument.empty() ? 0 : std::stoll(ent.argument));
        default:                               return nullptr;
    }
}

/**
 * @brief Converts a vector of TDLib textEntity objects into CppGram MessageEntity objects.
 * @param entities Vector of TDLib textEntity pointers.
 * @return Vector of converted MessageEntity domain models.
 */
inline std::vector<MessageEntity> convert_entities(
        const std::vector<td_api::object_ptr<td_api::textEntity>>& entities) {
    std::vector<MessageEntity> out;
    for (const auto& e : entities) {
        if (!e || !e->type_) continue;
        MessageEntity ent;
        ent.offset = e->offset_;
        ent.length = e->length_;
        ent.type = convert_entity_type(*e->type_);
        switch (e->type_->get_id()) {
            case td_api::textEntityTypeTextUrl::ID: {
                auto& tu = static_cast<const td_api::textEntityTypeTextUrl&>(*e->type_);
                ent.argument = tu.url_;
                break;
            }
            case td_api::textEntityTypePreCode::ID: {
                auto& pc = static_cast<const td_api::textEntityTypePreCode&>(*e->type_);
                ent.argument = pc.language_;
                break;
            }
            case td_api::textEntityTypeMentionName::ID: {
                auto& mn = static_cast<const td_api::textEntityTypeMentionName&>(*e->type_);
                ent.argument = std::to_string(mn.user_id_);
                break;
            }
            default: break;
        }
        out.push_back(std::move(ent));
    }
    return out;
}

/**
 * @brief Helper to construct a plain unformatted TDLib formattedText object.
 * @param s Raw text string.
 * @return TDLib formattedText object pointer.
 */
inline td_api::object_ptr<td_api::formattedText> make_text(const std::string& s) {
    auto ft = td_api::make_object<td_api::formattedText>();
    ft->text_ = s;
    return ft;
}

/**
 * @brief Maps CppGram ParseMode to the corresponding TDLib TextParseMode object.
 * @param mode The CppGram ParseMode.
 * @return TDLib TextParseMode object pointer, or nullptr for plain text.
 */
inline td_api::object_ptr<td_api::TextParseMode> convert_parse_mode(ParseMode mode) {
    switch (mode) {
        case ParseMode::Markdown:   return td_api::make_object<td_api::textParseModeMarkdown>(1);
        case ParseMode::MarkdownV2: return td_api::make_object<td_api::textParseModeMarkdown>(2);
        case ParseMode::HTML:       return td_api::make_object<td_api::textParseModeHTML>();
        default:                    return nullptr;
    }
}

/**
 * @brief Parses formatted text (Markdown/HTML) via TDLib ClientManager into a formattedText object.
 * @param s Text containing formatting markup.
 * @param mode Target parse mode (Markdown, MarkdownV2, or HTML).
 * @return TDLib formattedText object pointer with parsed text and entity ranges.
 */
inline td_api::object_ptr<td_api::formattedText> parse_text(const std::string& s, ParseMode mode) {
    if (s.empty() || mode == ParseMode::None) {
        return make_text(s);
    }
    auto pm = convert_parse_mode(mode);
    if (!pm) return make_text(s);

    auto parsed = td::ClientManager::execute(
        td_api::make_object<td_api::parseTextEntities>(s, std::move(pm)));
    if (parsed && parsed->get_id() == td_api::formattedText::ID) {
        return td_api::move_object_as<td_api::formattedText>(parsed);
    }
    return make_text(s);
}

/**
 * @brief Converts a domain FormattedText object into a TDLib formattedText instance.
 * @param ft Domain FormattedText containing string and entity list.
 * @return TDLib formattedText object pointer.
 */
inline td_api::object_ptr<td_api::formattedText> convert_formatted_text(const FormattedText& ft) {
    auto out = td_api::make_object<td_api::formattedText>();
    out->text_ = ft.text;
    for (const auto& e : ft.entities) {
        auto t = make_td_entity_type(e);
        if (t) {
            out->entities_.push_back(td_api::make_object<td_api::textEntity>(e.offset, e.length, std::move(t)));
        }
    }
    return out;
}

/**
 * @brief Converts a TDLib formattedText instance into a domain FormattedText object.
 * @param ft TDLib formattedText reference.
 * @return Domain FormattedText instance.
 */
inline FormattedText convert_td_formatted_text(const td_api::formattedText& ft) {
    FormattedText out;
    out.text = ft.text_;
    out.entities = convert_entities(ft.entities_);
    return out;
}

// ============================================================================
// Message Sending Options and Profile Photos
// ============================================================================

/**
 * @brief Converts SendMessageOptions into TDLib messageSendOptions.
 * @param opts Options including scheduling, notification silence, and background delivery.
 * @return TDLib messageSendOptions object pointer.
 */
inline td_api::object_ptr<td_api::messageSendOptions> convert_send_options(const SendMessageOptions& opts) {
    auto out = td_api::make_object<td_api::messageSendOptions>();
    out->disable_notification_ = opts.disable_notification;
    out->from_background_      = opts.from_background;
    if (opts.schedule_date.has_value()) {
        auto tt = std::chrono::system_clock::to_time_t(*opts.schedule_date);
        out->scheduling_state_ = td_api::make_object<td_api::messageSchedulingStateSendAtDate>(
            static_cast<std::int32_t>(tt));
    }
    return out;
}

/**
 * @brief Converts a TDLib chatPhotos collection into a UserProfilePhotos domain object.
 * @param photos TDLib chatPhotos object containing profile photo entries.
 * @return Domain UserProfilePhotos collection.
 */
inline UserProfilePhotos convert_user_profile_photos(const td_api::chatPhotos& photos) {
    UserProfilePhotos out;
    out.total_count = photos.total_count_;
    for (const auto& p : photos.photos_) {
        if (!p) continue;
        Photo photo;
        for (const auto& s : p->sizes_) {
            if (s) photo.sizes.push_back(convert_photo_size(*s));
        }
        if (!photo.empty()) out.photos.push_back(std::move(photo));
    }
    return out;
}

} // namespace cppgram::detail
