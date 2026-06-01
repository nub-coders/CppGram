#pragma once
#include <functional>
#include <string>
#include <concepts>
#include "cppgram/message.hpp"

namespace cppgram {

class MessageFilter {
public:
    template <typename F>
        requires std::invocable<F, const Message&>
    MessageFilter(F fn) : fn_(std::move(fn)) {}

    bool operator()(const Message& m) const { return fn_(m); }

    friend MessageFilter operator&&(MessageFilter a, MessageFilter b) {
        return [a = std::move(a), b = std::move(b)](const Message& m) {
            return a(m) && b(m);
        };
    }
    friend MessageFilter operator||(MessageFilter a, MessageFilter b) {
        return [a = std::move(a), b = std::move(b)](const Message& m) {
            return a(m) || b(m);
        };
    }
    friend MessageFilter operator!(MessageFilter f) {
        return [f = std::move(f)](const Message& m) { return !f(m); };
    }

private:
    std::function<bool(const Message&)> fn_;
};

namespace Filters {

MessageFilter command(const std::string& cmd);
MessageFilter privateChat();
MessageFilter group();
MessageFilter channel();
MessageFilter text();
MessageFilter media();
MessageFilter photo();
MessageFilter video();
MessageFilter document();
MessageFilter audio();
MessageFilter voice();
MessageFilter videoNote();
MessageFilter animation();
MessageFilter sticker();
MessageFilter pollMsg();
MessageFilter locationMsg();
MessageFilter contactMsg();
MessageFilter dice();
MessageFilter bot();
MessageFilter outgoing();
MessageFilter incoming();
MessageFilter reply();
MessageFilter forwarded();
MessageFilter regex(const std::string& pattern);
MessageFilter chatId(ChatId id);
MessageFilter userId(UserId id);

} // namespace Filters
} // namespace cppgram
