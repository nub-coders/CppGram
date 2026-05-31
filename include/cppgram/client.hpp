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

namespace cppgram {
class ClientImpl;   // pimpl — hides TDLib + threading entirely

class Client {
public:
    Client();                                            // bot-style ctor
    Client(std::int32_t api_id, std::string api_hash);   // user-style ctor
    ~Client();
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    // ---- Auth (implemented Phase 3) ----
    void login(const std::string& phone);
    void loginBot(const std::string& token);
    void logout();
    AuthState authState() const;

    // ---- Core API (implemented Phase 4) ----
    User              getMe();
    User              getUser(UserId id);
    Chat              getChat(ChatId id);
    std::vector<Message> getHistory(ChatId chat_id, int limit = 100);
    Message           sendMessage(ChatId chat_id, const std::string& text,
                                  std::optional<MessageId> reply_to = std::nullopt);
    void              editMessage(ChatId, MessageId, const std::string& text);
    void              deleteMessages(ChatId, std::vector<MessageId>);
    void              setReaction(ChatId, MessageId, const std::string& emoji);
    void              leaveChat(ChatId);
    void              joinChat(ChatId);
    std::vector<User> getChatMembers(ChatId);

    // ---- Event loop ----
    void run();      // blocking
    void stop();

    // Shared impl so entities can hold weak_ptr to it.
    std::shared_ptr<ClientImpl> _impl() const;

private:
    std::shared_ptr<ClientImpl> impl_;
};
} // namespace cppgram