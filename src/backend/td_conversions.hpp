#pragma once
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <memory>
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

// ---- ChatType ----
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

// ---- MediaType ----
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

// ---- PhotoSize ----
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

// ---- Media extraction ----
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
            if (ms.sticker_->full_type_) {
                if (ms.sticker_->full_type_->get_id() == td_api::stickerFullTypeRegular::ID) {
                    si.is_animated = false;
                    si.is_video = false;
                }
            }
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
            if (mp.poll_->question_) p.question = mp.poll_->question_->text_;
            for (auto& o : mp.poll_->options_) {
                if (o) {
                    PollOption po;
                    if (o->text_) po.text = o->text_->text_;
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

// ---- Inline keyboard ----
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

// ---- Build TDLib ReplyMarkup from CppGram types ----
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
                            nullptr, btn.switch_inline_query);
                } else if (!btn.switch_inline_query_current_chat.empty()) {
                    td_btn->type_ = td_api::make_object<
                        td_api::inlineKeyboardButtonTypeSwitchInline>(
                            td_api::make_object<td_api::targetChatCurrent>(),
                            btn.switch_inline_query_current_chat);
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
        markup->is_persistent_ = rk->is_persistent;
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

// ---- Build TDLib InlineKeyboard from CppGram InlineKeyboard ----
inline td_api::object_ptr<td_api::ReplyMarkup> build_inline_keyboard(
        const InlineKeyboard& ik) {
    ReplyMarkup rm = ik;
    return build_reply_markup(rm);
}

// ---- User ----
inline User convert_user(const td_api::user& u,
                         std::weak_ptr<IBackend> backend = {}) {
    User out;
    out.id           = u.id_;
    out.first_name   = u.first_name_;
    out.last_name    = u.last_name_;
    if (u.usernames_)
        out.username = u.usernames_->editable_username_;
    out.phone_number = u.phone_number_;
    out.is_bot       = u.type_ && u.type_->get_id() == td_api::userTypeBot::ID;
    out.is_premium   = u.is_premium_;
    out.is_verified  = u.is_verified_;
    out.is_scam      = u.is_scam_;
    out.is_fake      = u.is_fake_;
    out._client      = backend;
    return out;
}

// ---- Chat ----
inline Chat convert_chat(const td_api::chat& c,
                         std::weak_ptr<IBackend> backend = {}) {
    Chat out;
    out.id    = c.id_;
    out.title = c.title_;
    if (c.type_)
        out.type = convert_chat_type(*c.type_);
    out.has_protected_content = c.has_protected_content_;
    if (c.permissions_) {
        out.permissions.can_send_messages = c.permissions_->can_send_basic_messages_;
        out.permissions.can_send_media = c.permissions_->can_send_audios_ ||
            c.permissions_->can_send_documents_ || c.permissions_->can_send_photos_ ||
            c.permissions_->can_send_videos_;
        out.permissions.can_send_polls = c.permissions_->can_send_polls_;
        out.permissions.can_send_other = c.permissions_->can_send_other_messages_;
        out.permissions.can_add_web_page_previews = c.permissions_->can_add_link_previews_;
        out.permissions.can_change_info = c.permissions_->can_change_info_;
        out.permissions.can_invite_users = c.permissions_->can_invite_users_;
        out.permissions.can_pin_messages = c.permissions_->can_pin_messages_;
        out.permissions.can_manage_topics = c.permissions_->can_create_topics_;
    }
    out._client = backend;
    return out;
}

// ---- ChatMember ----
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

// ---- Build TDLib ChatPermissions ----
inline td_api::object_ptr<td_api::chatPermissions> build_chat_permissions(
        const ChatPermissions& p) {
    auto out = td_api::make_object<td_api::chatPermissions>();
    out->can_send_basic_messages_ = p.can_send_messages;
    out->can_send_audios_ = p.can_send_media;
    out->can_send_documents_ = p.can_send_media;
    out->can_send_photos_ = p.can_send_media;
    out->can_send_videos_ = p.can_send_media;
    out->can_send_video_notes_ = p.can_send_media;
    out->can_send_voice_notes_ = p.can_send_media;
    out->can_send_polls_ = p.can_send_polls;
    out->can_send_other_messages_ = p.can_send_other;
    out->can_add_link_previews_ = p.can_add_web_page_previews;
    out->can_change_info_ = p.can_change_info;
    out->can_invite_users_ = p.can_invite_users;
    out->can_pin_messages_ = p.can_pin_messages;
    out->can_create_topics_ = p.can_manage_topics;
    return out;
}

// ---- FileInfo ----
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

// ---- Message ----
inline Message convert_message(const td_api::message& m,
                               ChatType chat_type,
                               std::weak_ptr<IBackend> backend = {}) {
    Message out;
    out.id       = m.id_;
    out.chat_id  = m.chat_id_;
    out.chat_type = chat_type;
    out.outgoing = m.is_outgoing_;
    out.date     = std::chrono::system_clock::from_time_t(m.date_);

    if (m.content_) {
        // Text
        if (m.content_->get_id() == td_api::messageText::ID) {
            auto& mt = static_cast<const td_api::messageText&>(*m.content_);
            if (mt.text_) out.text = mt.text_->text_;
        }

        // Media type + details
        out.media = classify_media(*m.content_);
        extract_media(*m.content_, out);
    }

    // Reply markup
    out.reply_markup = convert_reply_markup(m.reply_markup_.get());

    // Reply
    if (m.reply_to_ && m.reply_to_->get_id() == td_api::messageReplyToMessage::ID) {
        auto& r = static_cast<const td_api::messageReplyToMessage&>(*m.reply_to_);
        out.reply_to = r.message_id_;
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
            case td_api::messageOriginUser::ID: {
                auto& o = static_cast<const td_api::messageOriginUser&>(*origin);
                fi.origin_sender_id = o.sender_user_id_;
                break;
            }
            case td_api::messageOriginChat::ID: {
                auto& o = static_cast<const td_api::messageOriginChat&>(*origin);
                fi.origin_chat_id = o.sender_chat_id_;
                fi.origin_sender_name = o.author_signature_;
                break;
            }
            case td_api::messageOriginChannel::ID: {
                auto& o = static_cast<const td_api::messageOriginChannel&>(*origin);
                fi.origin_chat_id    = o.chat_id_;
                fi.origin_message_id = o.message_id_;
                fi.origin_sender_name = o.author_signature_;
                break;
            }
            case td_api::messageOriginHiddenUser::ID: {
                auto& o = static_cast<const td_api::messageOriginHiddenUser&>(*origin);
                fi.origin_sender_name = o.sender_name_;
                break;
            }
            default: break;
        }
        out.forward_info = fi;
    }

    out._client = backend;
    return out;
}

// ---- CallbackQuery ----
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

// ---- Build TDLib formattedText ----
inline td_api::object_ptr<td_api::formattedText> make_text(const std::string& s) {
    auto ft = td_api::make_object<td_api::formattedText>();
    ft->text_ = s;
    return ft;
}

} // namespace cppgram::detail
