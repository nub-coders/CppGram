#pragma once
#include <string>
#include <memory>
#include "cppgram/types.hpp"

namespace cppgram {
class IBackend;

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
    bool        is_scam{false};
    bool        is_fake{false};

    std::string full_name() const {
        return last_name.empty() ? first_name : first_name + " " + last_name;
    }

    std::weak_ptr<IBackend> _client;
};

} // namespace cppgram
