#include "cppgram/client.hpp"
#include "cppgram/errors.hpp"
#include "cppgram/log.hpp"
#include <atomic>
#include <thread>

namespace cppgram {

class ClientImpl {
public:
    ApiCredentials creds;
    std::atomic<bool> running{false};
    AuthState auth{AuthState::None};

    // Phase 4 fills these with real backend round-trips.
    Message sendMessage(ChatId chat, const std::string& text,
                        std::optional<MessageId> reply_to) {
        CPPGRAM_INFO("client", "sendMessage chat={} reply_to={} text='{}'",
                     chat, reply_to.value_or(0), text);
        Message m; m.chat_id = chat; m.text = text; m.outgoing = true;
        m.reply_to = reply_to;
        return m;
    }
    void editMessage(ChatId, MessageId, const std::string&) {}
    void deleteMessages(ChatId, std::vector<MessageId>) {}
    void setReaction(ChatId, MessageId, const std::string&) {}
    void leaveChat(ChatId) {}
    void joinChat(ChatId) {}
    std::vector<User> getChatMembers(ChatId) { return {}; }
};

Client::Client() : impl_(std::make_shared<ClientImpl>()) {}
Client::Client(std::int32_t api_id, std::string api_hash)
    : impl_(std::make_shared<ClientImpl>()) {
    impl_->creds = {api_id, std::move(api_hash)};
}
Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

std::shared_ptr<ClientImpl> Client::_impl() const { return impl_; }

// ---- Auth stubs (Phase 3) ----
void Client::login(const std::string& phone) {
    CPPGRAM_INFO("auth", "login requested for {}", phone);
    throw AuthenticationError("login() not implemented until Phase 3");
}
void Client::loginBot(const std::string& token) {
    CPPGRAM_INFO("auth", "bot login (token len={})", token.size());
    throw AuthenticationError("loginBot() not implemented until Phase 3");
}
void Client::logout() { impl_->auth = AuthState::LoggingOut; }
AuthState Client::authState() const { return impl_->auth; }

// ---- Core API delegates ----
Message Client::sendMessage(ChatId c, const std::string& t, std::optional<MessageId> r) {
    auto m = impl_->sendMessage(c, t, r);
    m._client = impl_;
    return m;
}
void Client::editMessage(ChatId c, MessageId m, const std::string& t) { impl_->editMessage(c,m,t); }
void Client::deleteMessages(ChatId c, std::vector<MessageId> v) { impl_->deleteMessages(c,std::move(v)); }
void Client::setReaction(ChatId c, MessageId m, const std::string& e){ impl_->setReaction(c,m,e); }
void Client::leaveChat(ChatId c) { impl_->leaveChat(c); }
void Client::joinChat(ChatId c)  { impl_->joinChat(c); }
std::vector<User> Client::getChatMembers(ChatId c){ return impl_->getChatMembers(c); }

User Client::getMe()           { throw CppGramException("getMe() lands in Phase 4"); }
User Client::getUser(UserId)   { throw CppGramException("getUser() lands in Phase 4"); }
Chat Client::getChat(ChatId)   { throw CppGramException("getChat() lands in Phase 4"); }
std::vector<Message> Client::getHistory(ChatId, int) { return {}; }

void Client::run() {
    impl_->running = true;
    CPPGRAM_INFO("client", "event loop started (Phase 1 idle loop)");
    using namespace std::chrono_literals;
    while (impl_->running) std::this_thread::sleep_for(50ms);
}
void Client::stop() { impl_->running = false; }

} // namespace cppgram