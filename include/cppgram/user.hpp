#pragma once
#include <string>
#include <memory>
#include "cppgram/types.hpp"

namespace cppgram {
class ClientImpl;

class User {
public:
    UserId      id{};
    std::string first_name;
    std::string last_name;
    std::string username;
    std::string phone_number;
    bool        is_bot{false};
    bool        is_premium{false};
    bool        is_verified{false};

    std::string full_name() const {
        return last_name.empty() ? first_name : first_name + " " + last_name;
    }
    // Internal: lets entity methods reach the client without owning it.
    std::weak_ptr<ClientImpl> _client;
};
} // namespace cppgram