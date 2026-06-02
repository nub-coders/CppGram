#include "cppgram/client.hpp"
#include "cppgram/i_backend.hpp"
#include "cppgram/errors.hpp"
#include "cppgram/log.hpp"
#include "cppgram/handlers.hpp"
#include "cppgram/media.hpp"
#include "cppgram/keyboard.hpp"

#include "tdlib_adapter.hpp"
#include "td_conversions.hpp"

#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace cppgram {

namespace td_api = td::td_api;

// ---------------------------------------------------------------------------
// ClientImpl
// ---------------------------------------------------------------------------
class ClientImpl : public IBackend,
                   public std::enable_shared_from_this<ClientImpl> {
public:
    ApiCredentials creds;
    detail::TdLibAdapter td;

    std::atomic<AuthState> auth_state{AuthState::None};
    std::atomic<bool> event_loop_running{false};

    // Auth callbacks
    std::function<std::string()> code_callback_;
    std::function<std::string()> password_callback_;
    std::mutex auth_mtx_;
    std::condition_variable auth_cv_;

    // Event loop synchronization
    std::mutex run_mtx_;
    std::condition_variable run_cv_;

    // Caches
    std::mutex cache_mtx_;
    std::unordered_map<UserId, User>  user_cache_;
    std::unordered_map<ChatId, Chat>  chat_cache_;
    std::unordered_map<ChatId, ChatType> chat_type_cache_;

    // Event handlers
    std::mutex handlers_mtx_;
    std::vector<MessageHandler>         on_message_handlers_;
    std::vector<EditedMessageHandler>   on_edited_message_handlers_;
    std::vector<DeletedMessagesHandler> on_deleted_messages_handlers_;
    std::vector<CallbackQueryHandler>   on_callback_query_handlers_;

    // ---- Initialization ----
    void init() {
        td.set_update_handler([this](detail::Object obj) {
            process_update(std::move(obj));
        });
        td.start();
    }

    // ---- Auth helpers ----
    void wait_for_auth(AuthState target) {
        std::unique_lock lk(auth_mtx_);
        auth_cv_.wait(lk, [&] {
            auto s = auth_state.load();
            return s == target || s == AuthState::Closed;
        });
        if (auth_state.load() == AuthState::Closed)
            throw AuthenticationError("Authorization closed unexpectedly");
    }

    void send_tdlib_parameters() {
        auto params = td_api::make_object<td_api::tdlibParameters>();
        params->use_test_dc_ = false;
        params->database_directory_ = "cppgram_data";
        params->files_directory_ = "cppgram_data/files";
        params->use_file_database_ = true;
        params->use_chat_info_database_ = true;
        params->use_message_database_ = true;
        params->use_secret_chats_ = false;
        params->api_id_ = creds.api_id;
        params->api_hash_ = creds.api_hash;
        params->system_language_code_ = "en";
        params->device_model_ = "CppGram";
        params->system_version_ = "Linux";
        params->application_version_ = "0.1.0";
        params->enable_storage_optimizer_ = true;
        params->ignore_file_names_ = false;
        td.send(td_api::make_object<td_api::setTdlibParameters>(std::move(params)), {});
    }

    // ---- Chat type lookup helper ----
    ChatType lookup_chat_type(ChatId chat_id) {
        std::lock_guard lk(cache_mtx_);
        auto it = chat_type_cache_.find(chat_id);
        return it != chat_type_cache_.end() ? it->second : ChatType::Unknown;
    }

    // ---- Generic media send helper ----
    Message send_content(ChatId chat_id,
                         td_api::object_ptr<td_api::InputMessageContent> content,
                         std::optional<MessageId> reply_to,
                         td_api::object_ptr<td_api::ReplyMarkup> markup,
                         const char* ctx) {
        auto req = td_api::make_object<td_api::sendMessage>();
        req->chat_id_ = chat_id;
        req->message_thread_id_ = 0;
        req->reply_to_message_id_ = reply_to.value_or(0);
        req->options_ = nullptr;
        req->reply_markup_ = std::move(markup);
        req->input_message_content_ = std::move(content);

        auto result = td.send_sync(std::move(req));
        check_error(result, ctx);
        if (result && result->get_id() == td_api::message::ID) {
            auto& m = static_cast<td_api::message&>(*result);
            return detail::convert_message(m, lookup_chat_type(chat_id),
                                           weak_from_this());
        }
        Message stub;
        stub.chat_id = chat_id;
        stub.outgoing = true;
        stub._client = weak_from_this();
        return stub;
    }

    // ---- Update dispatch ----
    void process_update(detail::Object obj) {
        if (!obj) return;
        auto id = obj->get_id();

        switch (id) {
        case td_api::updateAuthorizationState::ID: {
            auto& upd = static_cast<td_api::updateAuthorizationState&>(*obj);
            handle_auth_state(std::move(upd.authorization_state_));
            break;
        }
        case td_api::updateUser::ID: {
            auto& upd = static_cast<td_api::updateUser&>(*obj);
            if (upd.user_) {
                std::lock_guard lk(cache_mtx_);
                user_cache_[upd.user_->id_] = detail::convert_user(*upd.user_);
            }
            break;
        }
        case td_api::updateNewChat::ID: {
            auto& upd = static_cast<td_api::updateNewChat&>(*obj);
            if (upd.chat_) {
                std::lock_guard lk(cache_mtx_);
                chat_cache_[upd.chat_->id_] = detail::convert_chat(*upd.chat_);
                if (upd.chat_->type_)
                    chat_type_cache_[upd.chat_->id_] =
                        detail::convert_chat_type(*upd.chat_->type_);
            }
            break;
        }
        case td_api::updateNewMessage::ID: {
            auto& upd = static_cast<td_api::updateNewMessage&>(*obj);
            if (upd.message_) {
                if (upd.message_->content_ &&
                    upd.message_->content_->get_id() == td_api::messageText::ID) {
                    auto& mt = static_cast<const td_api::messageText&>(
                        *upd.message_->content_);
                    CPPGRAM_INFO("update", "received new message chat={} id={} text={}",
                                 upd.message_->chat_id_, upd.message_->id_,
                                 mt.text_ ? mt.text_->text_ : "<empty>");
                } else {
                    CPPGRAM_INFO("update", "received new non-text message chat={} id={} type={}",
                                 upd.message_->chat_id_, upd.message_->id_,
                                 upd.message_->content_ ? upd.message_->content_->get_id() : 0);
                }
                auto msg = detail::convert_message(
                    *upd.message_, lookup_chat_type(upd.message_->chat_id_),
                    weak_from_this());
                enrich_sender(msg);
                dispatch_message(std::move(msg));
            }
            break;
        }
        case td_api::updateMessageContent::ID: {
            auto& upd = static_cast<td_api::updateMessageContent&>(*obj);
            Message msg;
            msg.id = upd.message_id_;
            msg.chat_id = upd.chat_id_;
            msg.chat_type = lookup_chat_type(upd.chat_id_);
            if (upd.new_content_) {
                msg.media = detail::classify_media(*upd.new_content_);
                if (upd.new_content_->get_id() == td_api::messageText::ID) {
                    auto& mt = static_cast<const td_api::messageText&>(
                        *upd.new_content_);
                    if (mt.text_) msg.text = mt.text_->text_;
                }
                detail::extract_media(*upd.new_content_, msg);
            }
            msg._client = weak_from_this();
            dispatch_edited_message(std::move(msg));
            break;
        }
        case td_api::updateDeleteMessages::ID: {
            auto& upd = static_cast<td_api::updateDeleteMessages&>(*obj);
            if (upd.is_permanent_) {
                dispatch_deleted_messages(upd.chat_id_,
                                          std::move(upd.message_ids_));
            }
            break;
        }
        case td_api::updateNewCallbackQuery::ID: {
            auto& upd = static_cast<td_api::updateNewCallbackQuery&>(*obj);
            auto q = detail::convert_callback_query(upd, weak_from_this());
            enrich_sender_by_id(q.sender);
            dispatch_callback_query(std::move(q));
            break;
        }
        default:
            break;
        }
    }

    void handle_auth_state(td_api::object_ptr<td_api::AuthorizationState> state) {
        if (!state) return;

        // Log the incoming authorization state for debugging hangs during login
        const char* state_name = "unknown";
        switch (state->get_id()) {
            case td_api::authorizationStateWaitTdlibParameters::ID:
                state_name = "authorizationStateWaitTdlibParameters"; break;
            case td_api::authorizationStateWaitEncryptionKey::ID:
                state_name = "authorizationStateWaitEncryptionKey"; break;
            case td_api::authorizationStateWaitPhoneNumber::ID:
                state_name = "authorizationStateWaitPhoneNumber"; break;
            case td_api::authorizationStateWaitCode::ID:
                state_name = "authorizationStateWaitCode"; break;
            case td_api::authorizationStateWaitPassword::ID:
                state_name = "authorizationStateWaitPassword"; break;
            case td_api::authorizationStateWaitRegistration::ID:
                state_name = "authorizationStateWaitRegistration"; break;
            case td_api::authorizationStateReady::ID:
                state_name = "authorizationStateReady"; break;
            case td_api::authorizationStateLoggingOut::ID:
                state_name = "authorizationStateLoggingOut"; break;
            case td_api::authorizationStateClosed::ID:
                state_name = "authorizationStateClosed"; break;
            default: break;
        }
        CPPGRAM_INFO("auth", "handle_auth_state: id={} name={}", state->get_id(), state_name);

        switch (state->get_id()) {
        case td_api::authorizationStateWaitTdlibParameters::ID:
            auth_state = AuthState::None;
            send_tdlib_parameters();
            break;
        case td_api::authorizationStateWaitPhoneNumber::ID:
            auth_state = AuthState::WaitPhoneNumber;
            auth_cv_.notify_all();
            break;
        case td_api::authorizationStateWaitEncryptionKey::ID: {
            auth_state = AuthState::WaitPassword;
            auth_cv_.notify_all();
            CPPGRAM_INFO("auth", "handle_auth_state: sending checkDatabaseEncryptionKey(empty)");
            td.send(td_api::make_object<td_api::checkDatabaseEncryptionKey>(""), {});
            break;
        }
        case td_api::authorizationStateWaitCode::ID: {
            auth_state = AuthState::WaitCode;
            auth_cv_.notify_all();
            if (code_callback_) {
                std::string code = code_callback_();
                td.send(td_api::make_object<td_api::checkAuthenticationCode>(code), {});
            }
            break;
        }
        case td_api::authorizationStateWaitPassword::ID: {
            auth_state = AuthState::WaitPassword;
            auth_cv_.notify_all();
            if (password_callback_) {
                std::string pw = password_callback_();
                td.send(td_api::make_object<td_api::checkAuthenticationPassword>(pw), {});
            } else {
                throw AuthenticationError(
                    "2FA password required but no callback provided");
            }
            break;
        }
        case td_api::authorizationStateWaitRegistration::ID:
            auth_state = AuthState::WaitRegistration;
            auth_cv_.notify_all();
            break;
        case td_api::authorizationStateReady::ID:
            CPPGRAM_INFO("auth", "Authorization successful");
            auth_state = AuthState::Ready;
            auth_cv_.notify_all();
            break;
        case td_api::authorizationStateLoggingOut::ID:
            auth_state = AuthState::LoggingOut;
            auth_cv_.notify_all();
            break;
        case td_api::authorizationStateClosed::ID:
            auth_state = AuthState::Closed;
            auth_cv_.notify_all();
            break;
        default:
            break;
        }
    }

    // ---- Sender enrichment ----
    void enrich_sender(Message& msg) {
        std::lock_guard lk(cache_mtx_);
        auto it = user_cache_.find(msg.sender.id);
        if (it != user_cache_.end()) {
            msg.sender = it->second;
            msg.sender._client = weak_from_this();
        }
    }

    void enrich_sender_by_id(User& u) {
        std::lock_guard lk(cache_mtx_);
        auto it = user_cache_.find(u.id);
        if (it != user_cache_.end()) {
            u = it->second;
            u._client = weak_from_this();
        }
    }

    // ---- Event dispatch ----
    void dispatch_message(Message msg) {
        std::vector<MessageHandler> copy;
        { std::lock_guard lk(handlers_mtx_); copy = on_message_handlers_; }
        CPPGRAM_INFO("dispatch", "dispatching message chat={} id={} text={} handlers={}",
                     msg.chat_id, msg.id, msg.text, copy.size());
        for (auto& h : copy) {
            if (h.filter(msg)) {
                CPPGRAM_INFO("dispatch", "handler matched for chat={} id={}", msg.chat_id, msg.id);
                try { h.callback(msg); }
                catch (const std::exception& e) {
                    CPPGRAM_ERROR("handler",
                                  "Exception in message handler: {}", e.what());
                }
            }
        }
    }

    void dispatch_edited_message(Message msg) {
        std::vector<EditedMessageHandler> copy;
        { std::lock_guard lk(handlers_mtx_); copy = on_edited_message_handlers_; }
        for (auto& h : copy) {
            if (h.filter(msg)) {
                try { h.callback(msg); }
                catch (const std::exception& e) {
                    CPPGRAM_ERROR("handler",
                                  "Exception in edited message handler: {}",
                                  e.what());
                }
            }
        }
    }

    void dispatch_deleted_messages(ChatId chat_id, std::vector<std::int64_t> ids) {
        std::vector<DeletedMessagesHandler> copy;
        { std::lock_guard lk(handlers_mtx_); copy = on_deleted_messages_handlers_; }
        for (auto& h : copy) {
            try { h.callback(chat_id, ids); }
            catch (const std::exception& e) {
                CPPGRAM_ERROR("handler",
                              "Exception in deleted messages handler: {}",
                              e.what());
            }
        }
    }

    void dispatch_callback_query(CallbackQuery q) {
        std::vector<CallbackQueryHandler> copy;
        { std::lock_guard lk(handlers_mtx_); copy = on_callback_query_handlers_; }
        for (auto& h : copy) {
            if (!h.filter || h.filter(q)) {
                try { h.callback(q); }
                catch (const std::exception& e) {
                    CPPGRAM_ERROR("handler",
                                  "Exception in callback query handler: {}",
                                  e.what());
                }
            }
        }
    }

    // ---- Error checking ----
    void check_error(const detail::Object& obj,
                     const char* context) {
        if (obj && obj->get_id() == td_api::error::ID) {
            auto& err = static_cast<const td_api::error&>(*obj);
            std::string msg = std::string(context) + ": " + err.message_ +
                              " (code " + std::to_string(err.code_) + ")";
            Error parsed = Error::from_rpc(err.code_, err.message_);
            parsed.message = msg;
            parsed.raise();
        }
    }

    // ==================================================================
    // IBackend implementation
    // ==================================================================

    // ---- Queries ----

    User getMe() override {
        auto result = td.send_sync(td_api::make_object<td_api::getMe>());
        check_error(result, "getMe");
        if (result && result->get_id() == td_api::user::ID) {
            auto& u = static_cast<td_api::user&>(*result);
            auto user = detail::convert_user(u, weak_from_this());
            std::lock_guard lk(cache_mtx_);
            user_cache_[user.id] = user;
            return user;
        }
        throw CppGramException("getMe: unexpected response");
    }

    User getUser(UserId id) override {
        {
            std::lock_guard lk(cache_mtx_);
            auto it = user_cache_.find(id);
            if (it != user_cache_.end()) return it->second;
        }
        auto result = td.send_sync(td_api::make_object<td_api::getUser>(id));
        check_error(result, "getUser");
        if (result && result->get_id() == td_api::user::ID) {
            auto& u = static_cast<td_api::user&>(*result);
            auto user = detail::convert_user(u, weak_from_this());
            std::lock_guard lk(cache_mtx_);
            user_cache_[user.id] = user;
            return user;
        }
        throw CppGramException("getUser: unexpected response");
    }

    Chat getChat(ChatId id) override {
        {
            std::lock_guard lk(cache_mtx_);
            auto it = chat_cache_.find(id);
            if (it != chat_cache_.end()) return it->second;
        }
        auto result = td.send_sync(td_api::make_object<td_api::getChat>(id));
        check_error(result, "getChat");
        if (result && result->get_id() == td_api::chat::ID) {
            auto& c = static_cast<td_api::chat&>(*result);
            auto chat = detail::convert_chat(c, weak_from_this());
            std::lock_guard lk(cache_mtx_);
            chat_cache_[chat.id] = chat;
            if (c.type_)
                chat_type_cache_[chat.id] = detail::convert_chat_type(*c.type_);
            return chat;
        }
        throw CppGramException("getChat: unexpected response");
    }

    std::vector<Message> getHistory(ChatId chat_id, int limit) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::getChatHistory>(
                chat_id, 0, 0, limit, false));
        check_error(result, "getHistory");
        std::vector<Message> out;
        if (result && result->get_id() == td_api::messages::ID) {
            auto& msgs = static_cast<td_api::messages&>(*result);
            auto ct = lookup_chat_type(chat_id);
            for (auto& m : msgs.messages_) {
                if (m) out.push_back(
                    detail::convert_message(*m, ct, weak_from_this()));
            }
        }
        return out;
    }

    // ---- Text messaging ----

    Message sendMessage(ChatId chat_id, const std::string& text,
                        std::optional<MessageId> reply_to) override {
        auto content = td_api::make_object<td_api::inputMessageText>();
        content->text_ = detail::make_text(text);
        CPPGRAM_INFO("client", "sendMessage chat={}", chat_id);
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendMessage");
    }

    Message sendMessageWithMarkup(ChatId chat_id, const std::string& text,
                                   std::optional<MessageId> reply_to,
                                   const ReplyMarkup& markup) override {
        auto content = td_api::make_object<td_api::inputMessageText>();
        content->text_ = detail::make_text(text);
        return send_content(chat_id, std::move(content), reply_to,
                            detail::build_reply_markup(markup), "sendMessage");
    }

    void editMessage(ChatId chat_id, MessageId msg_id,
                     const std::string& text) override {
        auto content = td_api::make_object<td_api::inputMessageText>();
        content->text_ = detail::make_text(text);
        auto result = td.send_sync(
            td_api::make_object<td_api::editMessageText>(
                chat_id, msg_id, nullptr, std::move(content)));
        check_error(result, "editMessage");
    }

    void editMessageCaption(ChatId chat_id, MessageId msg_id,
                            const std::string& caption) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::editMessageCaption>(
                chat_id, msg_id, nullptr, detail::make_text(caption)));
        check_error(result, "editMessageCaption");
    }

    void editMessageReplyMarkup(ChatId chat_id, MessageId msg_id,
                                const InlineKeyboard& markup) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::editMessageReplyMarkup>(
                chat_id, msg_id, detail::build_inline_keyboard(markup)));
        check_error(result, "editMessageReplyMarkup");
    }

    void deleteMessages(ChatId chat_id, std::vector<MessageId> ids) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::deleteMessages>(
                chat_id, std::move(ids), true));
        check_error(result, "deleteMessages");
    }

    void setReaction(ChatId chat_id, MessageId msg_id,
                     const std::string& emoji) override {
        throw std::runtime_error(
            "reactions are not supported by the linked TDLib version");
    }

    Message forwardMessage(ChatId from_chat, MessageId msg_id,
                           ChatId to_chat) override {
        std::vector<std::int64_t> ids = {msg_id};
        auto result = td.send_sync(
            td_api::make_object<td_api::forwardMessages>(
                to_chat, from_chat, std::move(ids), nullptr, false, false,
                false));
        check_error(result, "forwardMessage");
        if (result && result->get_id() == td_api::messages::ID) {
            auto& msgs = static_cast<td_api::messages&>(*result);
            if (!msgs.messages_.empty() && msgs.messages_[0])
                return detail::convert_message(
                    *msgs.messages_[0], lookup_chat_type(to_chat),
                    weak_from_this());
        }
        Message stub;
        stub.chat_id = to_chat;
        stub._client = weak_from_this();
        return stub;
    }

    void pinMessage(ChatId chat_id, MessageId msg_id,
                    bool disable_notification) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::pinChatMessage>(
                chat_id, msg_id, disable_notification, false));
        check_error(result, "pinMessage");
    }

    void unpinMessage(ChatId chat_id, MessageId msg_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::unpinChatMessage>(chat_id, msg_id));
        check_error(result, "unpinMessage");
    }

    void unpinAllMessages(ChatId chat_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::unpinAllChatMessages>(chat_id));
        check_error(result, "unpinAllMessages");
    }

    // ---- Media messaging ----

    Message sendPhoto(ChatId chat_id, const InputFile& file,
                      const std::string& caption,
                      std::optional<MessageId> reply_to) override {
        auto content = td_api::make_object<td_api::inputMessagePhoto>();
        content->photo_ = td_api::make_object<td_api::inputFileLocal>(file.path);
        if (!caption.empty()) content->caption_ = detail::make_text(caption);
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendPhoto");
    }

    Message sendVideo(ChatId chat_id, const InputFile& file,
                      const std::string& caption,
                      std::optional<MessageId> reply_to,
                      int width, int height, int duration) override {
        auto content = td_api::make_object<td_api::inputMessageVideo>();
        content->video_ = td_api::make_object<td_api::inputFileLocal>(file.path);
        if (!caption.empty()) content->caption_ = detail::make_text(caption);
        content->width_ = width;
        content->height_ = height;
        content->duration_ = duration;
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendVideo");
    }

    Message sendDocument(ChatId chat_id, const InputFile& file,
                         const std::string& caption,
                         std::optional<MessageId> reply_to) override {
        auto content = td_api::make_object<td_api::inputMessageDocument>();
        content->document_ = td_api::make_object<td_api::inputFileLocal>(
            file.path);
        if (!caption.empty()) content->caption_ = detail::make_text(caption);
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendDocument");
    }

    Message sendAudio(ChatId chat_id, const InputFile& file,
                      const std::string& caption,
                      std::optional<MessageId> reply_to,
                      int duration, const std::string& title,
                      const std::string& performer) override {
        auto content = td_api::make_object<td_api::inputMessageAudio>();
        content->audio_ = td_api::make_object<td_api::inputFileLocal>(file.path);
        if (!caption.empty()) content->caption_ = detail::make_text(caption);
        content->duration_ = duration;
        content->title_ = title;
        content->performer_ = performer;
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendAudio");
    }

    Message sendVoiceNote(ChatId chat_id, const InputFile& file,
                          const std::string& caption,
                          std::optional<MessageId> reply_to,
                          int duration) override {
        auto content = td_api::make_object<td_api::inputMessageVoiceNote>();
        content->voice_note_ = td_api::make_object<td_api::inputFileLocal>(
            file.path);
        if (!caption.empty()) content->caption_ = detail::make_text(caption);
        content->duration_ = duration;
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendVoiceNote");
    }

    Message sendVideoNote(ChatId chat_id, const InputFile& file,
                          std::optional<MessageId> reply_to,
                          int duration, int length) override {
        auto content = td_api::make_object<td_api::inputMessageVideoNote>();
        content->video_note_ = td_api::make_object<td_api::inputFileLocal>(
            file.path);
        content->duration_ = duration;
        content->length_ = length;
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendVideoNote");
    }

    Message sendAnimation(ChatId chat_id, const InputFile& file,
                          const std::string& caption,
                          std::optional<MessageId> reply_to,
                          int width, int height, int duration) override {
        auto content = td_api::make_object<td_api::inputMessageAnimation>();
        content->animation_ = td_api::make_object<td_api::inputFileLocal>(
            file.path);
        if (!caption.empty()) content->caption_ = detail::make_text(caption);
        content->width_ = width;
        content->height_ = height;
        content->duration_ = duration;
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendAnimation");
    }

    Message sendSticker(ChatId chat_id, const InputFile& file,
                        std::optional<MessageId> reply_to) override {
        auto content = td_api::make_object<td_api::inputMessageSticker>();
        content->sticker_ = td_api::make_object<td_api::inputFileLocal>(
            file.path);
        return send_content(chat_id, std::move(content), reply_to, nullptr,
                            "sendSticker");
    }

    // ---- Rich messages ----

    Message sendPoll(ChatId chat_id, const std::string& question,
                     std::vector<std::string> options,
                     const PollConfig& config) override {
        auto content = td_api::make_object<td_api::inputMessagePoll>();
        content->question_ = question;
        for (auto& o : options)
            content->options_.push_back(std::move(o));
        content->is_anonymous_ = config.is_anonymous;
        if (config.type == PollType::Quiz) {
            auto quiz = td_api::make_object<td_api::pollTypeQuiz>();
            quiz->correct_option_id_ = config.correct_option_id.value_or(0);
            if (!config.explanation.empty())
                quiz->explanation_ = detail::make_text(config.explanation);
            content->type_ = std::move(quiz);
        } else {
            auto regular = td_api::make_object<td_api::pollTypeRegular>();
            regular->allow_multiple_answers_ = config.allows_multiple_answers;
            content->type_ = std::move(regular);
        }
        if (config.open_period > 0) content->open_period_ = config.open_period;
        return send_content(chat_id, std::move(content), std::nullopt, nullptr,
                            "sendPoll");
    }

    Message sendDice(ChatId chat_id, const std::string& emoji) override {
        auto content = td_api::make_object<td_api::inputMessageDice>();
        content->emoji_ = emoji;
        return send_content(chat_id, std::move(content), std::nullopt, nullptr,
                            "sendDice");
    }

    Message sendContact(ChatId chat_id, const Contact& contact) override {
        auto c = td_api::make_object<td_api::contact>();
        c->phone_number_ = contact.phone_number;
        c->first_name_ = contact.first_name;
        c->last_name_ = contact.last_name;
        c->user_id_ = contact.user_id;
        c->vcard_ = contact.vcard;
        auto content = td_api::make_object<td_api::inputMessageContact>(
            std::move(c));
        return send_content(chat_id, std::move(content), std::nullopt, nullptr,
                            "sendContact");
    }

    Message sendLocation(ChatId chat_id, const Location& loc) override {
        auto l = td_api::make_object<td_api::location>(
            loc.latitude, loc.longitude, loc.horizontal_accuracy);
        auto content = td_api::make_object<td_api::inputMessageLocation>(
            std::move(l), loc.live_period, loc.heading,
            loc.proximity_alert_radius);
        return send_content(chat_id, std::move(content), std::nullopt, nullptr,
                            "sendLocation");
    }

    Message sendVenue(ChatId chat_id, const Venue& venue) override {
        auto l = td_api::make_object<td_api::location>(
            venue.location.latitude, venue.location.longitude,
            venue.location.horizontal_accuracy);
        auto v = td_api::make_object<td_api::venue>(
            std::move(l), venue.title, venue.address,
            venue.provider, venue.id, venue.type);
        auto content = td_api::make_object<td_api::inputMessageVenue>(
            std::move(v));
        return send_content(chat_id, std::move(content), std::nullopt, nullptr,
                            "sendVenue");
    }

    void stopPoll(ChatId chat_id, MessageId msg_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::stopPoll>(chat_id, msg_id, nullptr));
        check_error(result, "stopPoll");
    }

    // ---- Chat management ----

    void leaveChat(ChatId chat_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::leaveChat>(chat_id));
        check_error(result, "leaveChat");
    }

    void joinChat(ChatId chat_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::joinChat>(chat_id));
        check_error(result, "joinChat");
    }

    std::vector<User> getChatMembers(ChatId chat_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::searchChatMembers>(
                chat_id, "", 200, nullptr));
        check_error(result, "getChatMembers");
        std::vector<User> out;
        if (result && result->get_id() == td_api::chatMembers::ID) {
            auto& cm = static_cast<td_api::chatMembers&>(*result);
            for (auto& member : cm.members_) {
                if (member && member->member_id_ &&
                    member->member_id_->get_id() ==
                        td_api::messageSenderUser::ID) {
                    auto& su = static_cast<td_api::messageSenderUser&>(
                        *member->member_id_);
                    std::lock_guard lk(cache_mtx_);
                    auto it = user_cache_.find(su.user_id_);
                    if (it != user_cache_.end())
                        out.push_back(it->second);
                    else {
                        User u;
                        u.id = su.user_id_;
                        out.push_back(u);
                    }
                }
            }
        }
        return out;
    }

    ChatId createGroup(const std::string& title,
                       std::vector<UserId> members) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::createNewBasicGroupChat>(
                std::move(members), title));
        check_error(result, "createGroup");
        if (result && result->get_id() == td_api::chat::ID) {
            auto& c = static_cast<td_api::chat&>(*result);
            return c.id_;
        }
        throw CppGramException("createGroup: unexpected response");
    }

    ChatId createSupergroup(const std::string& title, bool is_channel,
                            const std::string& description) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::createNewSupergroupChat>(
                title, is_channel, description, nullptr, false));
        check_error(result, "createSupergroup");
        if (result && result->get_id() == td_api::chat::ID) {
            auto& c = static_cast<td_api::chat&>(*result);
            return c.id_;
        }
        throw CppGramException("createSupergroup: unexpected response");
    }

    void setChatTitle(ChatId chat_id, const std::string& title) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatTitle>(chat_id, title));
        check_error(result, "setChatTitle");
    }

    void setChatDescription(ChatId chat_id,
                            const std::string& description) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatDescription>(
                chat_id, description));
        check_error(result, "setChatDescription");
    }

    void setChatPhoto(ChatId chat_id, const InputFile& file) override {
        auto photo = td_api::make_object<td_api::inputChatPhotoStatic>(
            td_api::make_object<td_api::inputFileLocal>(file.path));
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatPhoto>(
                chat_id, std::move(photo)));
        check_error(result, "setChatPhoto");
    }

    void deleteChatPhoto(ChatId chat_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatPhoto>(chat_id, nullptr));
        check_error(result, "deleteChatPhoto");
    }

    void setChatPermissions(ChatId chat_id,
                            const ChatPermissions& perms) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatPermissions>(
                chat_id, detail::build_chat_permissions(perms)));
        check_error(result, "setChatPermissions");
    }

    void banChatMember(ChatId chat_id, UserId user_id) override {
        auto member_id = td_api::make_object<td_api::messageSenderUser>(
            user_id);
        auto status = td_api::make_object<td_api::chatMemberStatusBanned>();
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatMemberStatus>(
                chat_id, std::move(member_id), std::move(status)));
        check_error(result, "banChatMember");
    }

    void unbanChatMember(ChatId chat_id, UserId user_id) override {
        auto member_id = td_api::make_object<td_api::messageSenderUser>(
            user_id);
        auto status = td_api::make_object<td_api::chatMemberStatusMember>();
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatMemberStatus>(
                chat_id, std::move(member_id), std::move(status)));
        check_error(result, "unbanChatMember");
    }

    void restrictChatMember(ChatId chat_id, UserId user_id,
                            const ChatPermissions& perms) override {
        auto member_id = td_api::make_object<td_api::messageSenderUser>(
            user_id);
        auto status = td_api::make_object<td_api::chatMemberStatusRestricted>();
        status->is_member_ = true;
        status->permissions_ = detail::build_chat_permissions(perms);
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatMemberStatus>(
                chat_id, std::move(member_id), std::move(status)));
        check_error(result, "restrictChatMember");
    }

    void promoteChatMember(ChatId chat_id, UserId user_id,
                           const ChatAdminRights& rights) override {
        auto member_id = td_api::make_object<td_api::messageSenderUser>(
            user_id);
        auto status = td_api::make_object<td_api::chatMemberStatusAdministrator>();
        status->custom_title_ = rights.custom_title;
        status->can_manage_chat_ = rights.can_manage_chat;
        status->can_change_info_ = rights.can_change_info;
        status->can_post_messages_ = rights.can_post_messages;
        status->can_edit_messages_ = rights.can_edit_messages;
        status->can_delete_messages_ = rights.can_delete_messages;
        status->can_invite_users_ = rights.can_invite_users;
        status->can_restrict_members_ = rights.can_restrict_members;
        status->can_pin_messages_ = rights.can_pin_messages;
        status->can_promote_members_ = rights.can_promote_members;
        status->can_manage_video_chats_ = rights.can_manage_video_chats;
        status->is_anonymous_ = rights.is_anonymous;
        auto result = td.send_sync(
            td_api::make_object<td_api::setChatMemberStatus>(
                chat_id, std::move(member_id), std::move(status)));
        check_error(result, "promoteChatMember");
    }

    ChatMember getChatMember(ChatId chat_id, UserId user_id) override {
        auto member_id = td_api::make_object<td_api::messageSenderUser>(
            user_id);
        auto result = td.send_sync(
            td_api::make_object<td_api::getChatMember>(
                chat_id, std::move(member_id)));
        check_error(result, "getChatMember");
        ChatMember cm;
        if (result && result->get_id() == td_api::chatMember::ID) {
            auto& m = static_cast<td_api::chatMember&>(*result);
            cm.user.id = user_id;
            {
                std::lock_guard lk(cache_mtx_);
                auto it = user_cache_.find(user_id);
                if (it != user_cache_.end()) cm.user = it->second;
            }
            if (m.status_) cm.status = detail::convert_member_status(*m.status_);
            cm.joined_date = std::chrono::system_clock::from_time_t(
                m.joined_chat_date_);
        }
        return cm;
    }

    int getChatMemberCount(ChatId chat_id) override {
        auto chat_result = td.send_sync(
            td_api::make_object<td_api::getChat>(chat_id));
        check_error(chat_result, "getChatMemberCount");
        // Try to get from supergroup full info for accurate count
        if (chat_result && chat_result->get_id() == td_api::chat::ID) {
            auto& c = static_cast<td_api::chat&>(*chat_result);
            if (c.type_ &&
                c.type_->get_id() == td_api::chatTypeSupergroup::ID) {
                auto& sg = static_cast<td_api::chatTypeSupergroup&>(*c.type_);
                auto fi_result = td.send_sync(
                    td_api::make_object<td_api::getSupergroupFullInfo>(
                        sg.supergroup_id_));
                if (fi_result &&
                    fi_result->get_id() == td_api::supergroupFullInfo::ID) {
                    auto& fi = static_cast<td_api::supergroupFullInfo&>(
                        *fi_result);
                    return fi.member_count_;
                }
            }
        }
        // Fallback: search members and return count
        auto result = td.send_sync(
            td_api::make_object<td_api::searchChatMembers>(
                chat_id, "", 1, nullptr));
        if (result && result->get_id() == td_api::chatMembers::ID) {
            auto& cm = static_cast<td_api::chatMembers&>(*result);
            return cm.total_count_;
        }
        return 0;
    }

    std::vector<ChatMember> getChatAdministrators(ChatId chat_id) override {
        auto filter = td_api::make_object<
            td_api::chatMembersFilterAdministrators>();
        auto result = td.send_sync(
            td_api::make_object<td_api::searchChatMembers>(
                chat_id, "", 200, std::move(filter)));
        check_error(result, "getChatAdministrators");
        std::vector<ChatMember> out;
        if (result && result->get_id() == td_api::chatMembers::ID) {
            auto& cm = static_cast<td_api::chatMembers&>(*result);
            for (auto& member : cm.members_) {
                if (!member) continue;
                ChatMember cpm;
                if (member->member_id_ &&
                    member->member_id_->get_id() ==
                        td_api::messageSenderUser::ID) {
                    auto& su = static_cast<td_api::messageSenderUser&>(
                        *member->member_id_);
                    cpm.user.id = su.user_id_;
                    std::lock_guard lk(cache_mtx_);
                    auto it = user_cache_.find(su.user_id_);
                    if (it != user_cache_.end()) cpm.user = it->second;
                }
                if (member->status_)
                    cpm.status = detail::convert_member_status(*member->status_);
                cpm.joined_date = std::chrono::system_clock::from_time_t(
                    member->joined_chat_date_);
                out.push_back(std::move(cpm));
            }
        }
        return out;
    }

    std::string getChatInviteLink(ChatId chat_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::replacePrimaryChatInviteLink>(
                chat_id));
        check_error(result, "getChatInviteLink");
        if (result && result->get_id() == td_api::chatInviteLink::ID) {
            auto& link = static_cast<td_api::chatInviteLink&>(*result);
            return link.invite_link_;
        }
        return "";
    }

    // ---- User operations ----

    void blockUser(UserId user_id) override {
        auto sender = td_api::make_object<td_api::messageSenderUser>(user_id);
        auto result = td.send_sync(
            td_api::make_object<td_api::toggleMessageSenderIsBlocked>(
                std::move(sender), true));
        check_error(result, "blockUser");
    }

    void unblockUser(UserId user_id) override {
        auto sender = td_api::make_object<td_api::messageSenderUser>(user_id);
        auto result = td.send_sync(
            td_api::make_object<td_api::toggleMessageSenderIsBlocked>(
                std::move(sender), false));
        check_error(result, "unblockUser");
    }

    // ---- File operations ----

    FileInfo getFile(FileId file_id) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::getFile>(file_id));
        check_error(result, "getFile");
        if (result && result->get_id() == td_api::file::ID) {
            auto& f = static_cast<td_api::file&>(*result);
            return detail::convert_file(f);
        }
        throw CppGramException("getFile: unexpected response");
    }

    std::string downloadFile(FileId file_id, const std::string& directory,
                             std::function<void(FileProgress)> progress) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::downloadFile>(
                file_id, 32, 0, 0, true));
        check_error(result, "downloadFile");
        if (result && result->get_id() == td_api::file::ID) {
            auto& f = static_cast<td_api::file&>(*result);
            if (f.local_ && f.local_->is_downloading_completed_) {
                if (progress) {
                    FileProgress fp;
                    fp.total = f.size_;
                    fp.downloaded = f.size_;
                    progress(fp);
                }
                return f.local_->path_;
            }
        }
        return "";
    }

    // ---- Search ----

    std::vector<Message> searchMessages(const std::string& query,
                                        int limit) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::searchMessages>(
                nullptr, query, 0, 0, 0, limit, nullptr, 0, 0));
        check_error(result, "searchMessages");
        std::vector<Message> out;
        if (result && result->get_id() == td_api::foundMessages::ID) {
            auto& fm = static_cast<td_api::foundMessages&>(*result);
            for (auto& m : fm.messages_) {
                if (m) {
                    auto ct = lookup_chat_type(m->chat_id_);
                    out.push_back(detail::convert_message(
                        *m, ct, weak_from_this()));
                }
            }
        }
        return out;
    }

    std::vector<Message> searchChatMessages(ChatId chat_id,
                                            const std::string& query,
                                            int limit) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::searchChatMessages>(
                chat_id, query, nullptr, 0, 0, limit, nullptr, 0));
        check_error(result, "searchChatMessages");
        std::vector<Message> out;
        if (result && result->get_id() == td_api::foundMessages::ID) {
            auto& fm = static_cast<td_api::foundMessages&>(*result);
            auto ct = lookup_chat_type(chat_id);
            for (auto& m : fm.messages_) {
                if (m) out.push_back(
                    detail::convert_message(*m, ct, weak_from_this()));
            }
        }
        return out;
    }

    // ---- Callback queries ----

    void answerCallbackQuery(std::int64_t query_id, const std::string& text,
                             bool show_alert, const std::string& url,
                             int cache_time) override {
        auto result = td.send_sync(
            td_api::make_object<td_api::answerCallbackQuery>(
                query_id, text, show_alert, url, cache_time));
        check_error(result, "answerCallbackQuery");
    }
};

// ---------------------------------------------------------------------------
// Client — public API
// ---------------------------------------------------------------------------

Client::Client() : impl_(std::make_shared<ClientImpl>()) {
    impl_->init();
}

Client::Client(std::int32_t api_id, std::string api_hash)
    : impl_(std::make_shared<ClientImpl>()) {
    impl_->creds = {api_id, std::move(api_hash)};
    impl_->init();
}

Client::~Client() {
    if (impl_) impl_->td.stop();
}

Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

// ---- Auth ----
void Client::login(const std::string& phone,
                   std::function<std::string()> code_callback,
                   std::function<std::string()> password_callback) {
    impl_->code_callback_     = std::move(code_callback);
    impl_->password_callback_ = std::move(password_callback);
    impl_->wait_for_auth(AuthState::WaitPhoneNumber);

    CPPGRAM_INFO("auth", "login requested for {}", phone);
    impl_->td.send(
        td_api::make_object<td_api::setAuthenticationPhoneNumber>(phone, nullptr),
        {});

    impl_->wait_for_auth(AuthState::Ready);
    CPPGRAM_INFO("auth", "login complete for {}", phone);
}

void Client::loginBot(const std::string& token) {
    CPPGRAM_INFO("auth", "loginBot: waiting for authorization state WaitPhoneNumber");
    impl_->wait_for_auth(AuthState::WaitPhoneNumber);

    CPPGRAM_INFO("auth", "bot login (token len={})", token.size());
    impl_->td.send(
        td_api::make_object<td_api::checkAuthenticationBotToken>(token), {});

    CPPGRAM_INFO("auth", "loginBot: waiting for authorization state Ready");
    impl_->wait_for_auth(AuthState::Ready);
    CPPGRAM_INFO("auth", "bot login complete");
}

void Client::logout() {
    impl_->td.send(td_api::make_object<td_api::logOut>(), {});
    impl_->auth_state = AuthState::LoggingOut;
}

AuthState Client::authState() const { return impl_->auth_state.load(); }

// ---- Queries ----
User Client::getMe()            { return impl_->getMe(); }
User Client::getUser(UserId id) { return impl_->getUser(id); }
Chat Client::getChat(ChatId id) { return impl_->getChat(id); }
std::vector<Message> Client::getHistory(ChatId c, int l) {
    return impl_->getHistory(c, l);
}

// ---- Text messaging ----
Message Client::sendMessage(ChatId c, const std::string& t,
                            std::optional<MessageId> r) {
    return impl_->sendMessage(c, t, r);
}
Message Client::sendMessage(ChatId c, const std::string& t,
                            const ReplyMarkup& m,
                            std::optional<MessageId> r) {
    return impl_->sendMessageWithMarkup(c, t, r, m);
}
void Client::editMessage(ChatId c, MessageId m, const std::string& t) {
    impl_->editMessage(c, m, t);
}
void Client::editMessageCaption(ChatId c, MessageId m, const std::string& t) {
    impl_->editMessageCaption(c, m, t);
}
void Client::editMessageReplyMarkup(ChatId c, MessageId m,
                                    const InlineKeyboard& k) {
    impl_->editMessageReplyMarkup(c, m, k);
}
void Client::deleteMessages(ChatId c, std::vector<MessageId> v) {
    impl_->deleteMessages(c, std::move(v));
}
void Client::setReaction(ChatId c, MessageId m, const std::string& e) {
    impl_->setReaction(c, m, e);
}
Message Client::forwardMessage(ChatId f, MessageId m, ChatId t) {
    return impl_->forwardMessage(f, m, t);
}
void Client::pinMessage(ChatId c, MessageId m, bool d) {
    impl_->pinMessage(c, m, d);
}
void Client::unpinMessage(ChatId c, MessageId m) {
    impl_->unpinMessage(c, m);
}
void Client::unpinAllMessages(ChatId c) {
    impl_->unpinAllMessages(c);
}

// ---- Media messaging ----
Message Client::sendPhoto(ChatId c, const InputFile& f, const std::string& cap,
                          std::optional<MessageId> r) {
    return impl_->sendPhoto(c, f, cap, r);
}
Message Client::sendVideo(ChatId c, const InputFile& f, const std::string& cap,
                          std::optional<MessageId> r,
                          int w, int h, int d) {
    return impl_->sendVideo(c, f, cap, r, w, h, d);
}
Message Client::sendDocument(ChatId c, const InputFile& f,
                             const std::string& cap,
                             std::optional<MessageId> r) {
    return impl_->sendDocument(c, f, cap, r);
}
Message Client::sendAudio(ChatId c, const InputFile& f, const std::string& cap,
                          std::optional<MessageId> r, int dur,
                          const std::string& title,
                          const std::string& performer) {
    return impl_->sendAudio(c, f, cap, r, dur, title, performer);
}
Message Client::sendVoiceNote(ChatId c, const InputFile& f,
                              const std::string& cap,
                              std::optional<MessageId> r, int d) {
    return impl_->sendVoiceNote(c, f, cap, r, d);
}
Message Client::sendVideoNote(ChatId c, const InputFile& f,
                              std::optional<MessageId> r, int d, int l) {
    return impl_->sendVideoNote(c, f, r, d, l);
}
Message Client::sendAnimation(ChatId c, const InputFile& f,
                              const std::string& cap,
                              std::optional<MessageId> r,
                              int w, int h, int d) {
    return impl_->sendAnimation(c, f, cap, r, w, h, d);
}
Message Client::sendSticker(ChatId c, const InputFile& f,
                            std::optional<MessageId> r) {
    return impl_->sendSticker(c, f, r);
}

// ---- Rich messages ----
Message Client::sendPoll(ChatId c, const std::string& q,
                         std::vector<std::string> o, const PollConfig& cfg) {
    return impl_->sendPoll(c, q, std::move(o), cfg);
}
Message Client::sendDice(ChatId c, const std::string& e) {
    return impl_->sendDice(c, e);
}
Message Client::sendContact(ChatId c, const Contact& co) {
    return impl_->sendContact(c, co);
}
Message Client::sendLocation(ChatId c, const Location& l) {
    return impl_->sendLocation(c, l);
}
Message Client::sendVenue(ChatId c, const Venue& v) {
    return impl_->sendVenue(c, v);
}
void Client::stopPoll(ChatId c, MessageId m) {
    impl_->stopPoll(c, m);
}

// ---- Chat management ----
void Client::leaveChat(ChatId c) { impl_->leaveChat(c); }
void Client::joinChat(ChatId c)  { impl_->joinChat(c); }
std::vector<User> Client::getChatMembers(ChatId c) {
    return impl_->getChatMembers(c);
}
ChatId Client::createGroup(const std::string& t, std::vector<UserId> m) {
    return impl_->createGroup(t, std::move(m));
}
ChatId Client::createSupergroup(const std::string& t, bool ch,
                                const std::string& d) {
    return impl_->createSupergroup(t, ch, d);
}
void Client::setChatTitle(ChatId c, const std::string& t) {
    impl_->setChatTitle(c, t);
}
void Client::setChatDescription(ChatId c, const std::string& d) {
    impl_->setChatDescription(c, d);
}
void Client::setChatPhoto(ChatId c, const InputFile& f) {
    impl_->setChatPhoto(c, f);
}
void Client::deleteChatPhoto(ChatId c) {
    impl_->deleteChatPhoto(c);
}
void Client::setChatPermissions(ChatId c, const ChatPermissions& p) {
    impl_->setChatPermissions(c, p);
}
void Client::banChatMember(ChatId c, UserId u) {
    impl_->banChatMember(c, u);
}
void Client::unbanChatMember(ChatId c, UserId u) {
    impl_->unbanChatMember(c, u);
}
void Client::restrictChatMember(ChatId c, UserId u,
                                const ChatPermissions& p) {
    impl_->restrictChatMember(c, u, p);
}
void Client::promoteChatMember(ChatId c, UserId u,
                               const ChatAdminRights& r) {
    impl_->promoteChatMember(c, u, r);
}
ChatMember Client::getChatMember(ChatId c, UserId u) {
    return impl_->getChatMember(c, u);
}
int Client::getChatMemberCount(ChatId c) {
    return impl_->getChatMemberCount(c);
}
std::vector<ChatMember> Client::getChatAdministrators(ChatId c) {
    return impl_->getChatAdministrators(c);
}
std::string Client::getChatInviteLink(ChatId c) {
    return impl_->getChatInviteLink(c);
}

// ---- User operations ----
void Client::blockUser(UserId u) { impl_->blockUser(u); }
void Client::unblockUser(UserId u) { impl_->unblockUser(u); }

// ---- File operations ----
FileInfo Client::getFile(FileId f) { return impl_->getFile(f); }
std::string Client::downloadFile(FileId f, const std::string& d,
                                 std::function<void(FileProgress)> p) {
    return impl_->downloadFile(f, d, std::move(p));
}

// ---- Search ----
std::vector<Message> Client::searchMessages(const std::string& q, int l) {
    return impl_->searchMessages(q, l);
}
std::vector<Message> Client::searchChatMessages(ChatId c, const std::string& q,
                                                int l) {
    return impl_->searchChatMessages(c, q, l);
}

// ---- Callback queries ----
void Client::answerCallbackQuery(std::int64_t id, const std::string& text,
                                 bool show_alert, const std::string& url,
                                 int cache_time) {
    impl_->answerCallbackQuery(id, text, show_alert, url, cache_time);
}

// ---- Event handlers ----
void Client::onMessage(MessageCallback cb) {
    std::lock_guard lk(impl_->handlers_mtx_);
    impl_->on_message_handlers_.push_back(
        {MessageFilter([](const Message&) { return true; }), std::move(cb)});
}

void Client::onMessage(MessageFilter filter, MessageCallback cb) {
    std::lock_guard lk(impl_->handlers_mtx_);
    impl_->on_message_handlers_.push_back({std::move(filter), std::move(cb)});
}

void Client::onEditedMessage(MessageCallback cb) {
    std::lock_guard lk(impl_->handlers_mtx_);
    impl_->on_edited_message_handlers_.push_back(
        {MessageFilter([](const Message&) { return true; }), std::move(cb)});
}

void Client::onEditedMessage(MessageFilter filter, MessageCallback cb) {
    std::lock_guard lk(impl_->handlers_mtx_);
    impl_->on_edited_message_handlers_.push_back(
        {std::move(filter), std::move(cb)});
}

void Client::onDeletedMessages(
        std::function<void(ChatId, std::vector<MessageId>)> cb) {
    std::lock_guard lk(impl_->handlers_mtx_);
    impl_->on_deleted_messages_handlers_.push_back({std::move(cb)});
}

void Client::onCallbackQuery(std::function<void(CallbackQuery)> cb) {
    std::lock_guard lk(impl_->handlers_mtx_);
    impl_->on_callback_query_handlers_.push_back({nullptr, std::move(cb)});
}

void Client::onCallbackQuery(
        std::function<bool(const CallbackQuery&)> filter,
        std::function<void(CallbackQuery)> cb) {
    std::lock_guard lk(impl_->handlers_mtx_);
    impl_->on_callback_query_handlers_.push_back(
        {std::move(filter), std::move(cb)});
}

// ---- Event loop (fixed: uses condition_variable instead of busy-wait) ----
void Client::run() {
    impl_->event_loop_running = true;
    CPPGRAM_INFO("client", "event loop started (blocking)");
    std::unique_lock lk(impl_->run_mtx_);
    impl_->run_cv_.wait(lk, [&] {
        return !impl_->event_loop_running.load();
    });
}

void Client::stop() {
    impl_->event_loop_running = false;
    impl_->run_cv_.notify_all();
}

} // namespace cppgram
