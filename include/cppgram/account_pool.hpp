#pragma once

#include "cppgram/client.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>

namespace cppgram {

class AccountPool {
public:
    AccountPool() = default;

    void add_account(const std::string& account_id, std::shared_ptr<Client> client);
    bool remove_account(const std::string& account_id);
    [[nodiscard]] bool has_account(const std::string& account_id) const;

    [[nodiscard]] std::shared_ptr<Client> get_account(const std::string& account_id) const;
    [[nodiscard]] std::shared_ptr<Client> get_next_account();

    [[nodiscard]] size_t size() const;
    [[nodiscard]] std::vector<std::string> get_all_account_ids() const;

    std::vector<MessageId> broadcast(
        const std::vector<ChatId>& chat_ids,
        const std::string& text,
        ParseMode parse_mode = ParseMode::None);

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<Client>> accounts_;
    std::vector<std::string> account_keys_;
    std::atomic<size_t> round_robin_index_{0};
};

} // namespace cppgram
