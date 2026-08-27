#include "cppgram/account_pool.hpp"
#include <algorithm>

namespace cppgram {

void AccountPool::add_account(const std::string& account_id, std::shared_ptr<Client> client) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (accounts_.find(account_id) == accounts_.end()) {
        account_keys_.push_back(account_id);
    }
    accounts_[account_id] = std::move(client);
}

bool AccountPool::remove_account(const std::string& account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = accounts_.find(account_id);
    if (it == accounts_.end()) {
        return false;
    }
    accounts_.erase(it);
    account_keys_.erase(
        std::remove(account_keys_.begin(), account_keys_.end(), account_id),
        account_keys_.end());
    return true;
}

bool AccountPool::has_account(const std::string& account_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return accounts_.find(account_id) != accounts_.end();
}

std::shared_ptr<Client> AccountPool::get_account(const std::string& account_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = accounts_.find(account_id);
    if (it != accounts_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Client> AccountPool::get_next_account() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (account_keys_.empty()) {
        return nullptr;
    }
    size_t idx = round_robin_index_++ % account_keys_.size();
    return accounts_[account_keys_[idx]];
}

size_t AccountPool::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return accounts_.size();
}

std::vector<std::string> AccountPool::get_all_account_ids() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return account_keys_;
}

std::vector<MessageId> AccountPool::broadcast(
    const std::vector<ChatId>& chat_ids,
    const std::string& text,
    ParseMode parse_mode) {
    std::vector<MessageId> sent_ids;
    sent_ids.reserve(chat_ids.size());

    for (ChatId chat_id : chat_ids) {
        auto client = get_next_account();
        if (client) {
            try {
                auto msg = client->sendMessage(chat_id, text, parse_mode);
                sent_ids.push_back(msg.id);
            } catch (...) {
                // Continue to next account / chat
            }
        }
    }
    return sent_ids;
}

} // namespace cppgram
