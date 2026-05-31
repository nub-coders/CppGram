#pragma once
#include <string>
#include <vector>
#include <memory>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"

namespace cppgram {
class ClientImpl;
class Message;

class Chat {
public:
    ChatId      id{};
    ChatType    type{ChatType::Unknown};
    std::string title;
    std::string username;
    int         member_count{0};

    Message sendMessage(const std::string& text);   // defined in entities.cpp
    void    leave();
    void    join();
    std::vector<User> members();

    std::weak_ptr<ClientImpl> _client;
};
} // namespace cppgram