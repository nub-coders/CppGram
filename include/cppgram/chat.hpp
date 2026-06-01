#pragma once
#include <string>
#include <vector>
#include <memory>
#include "cppgram/types.hpp"
#include "cppgram/user.hpp"

namespace cppgram {
class IBackend;
class Message;

struct ChatMember {
    User user;
    ChatMemberStatus status{ChatMemberStatus::Member};
    Timestamp joined_date{};
    ChatAdminRights admin_rights;
    ChatPermissions permissions;
};

class Chat {
public:
    ChatId      id{};
    ChatType    type{ChatType::Unknown};
    std::string title;
    std::string username;
    std::string description;
    std::string invite_link;
    int         member_count{0};
    ChatPermissions permissions;
    bool has_protected_content{false};

    Message sendMessage(const std::string& text);
    void    leave();
    void    join();
    std::vector<User> members();
    void setTitle(const std::string& new_title);
    void setDescription(const std::string& desc);
    void setPermissions(const ChatPermissions& perms);
    void banMember(UserId user_id);
    void unbanMember(UserId user_id);
    void promoteMember(UserId user_id, const ChatAdminRights& rights);
    void restrictMember(UserId user_id, const ChatPermissions& perms);
    std::string getInviteLink();
    int getMemberCount();

    std::weak_ptr<IBackend> _client;
};

} // namespace cppgram
