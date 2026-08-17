#pragma once

/**
 * @file secret_chat.hpp
 * @brief End-to-end encrypted secret chat entities and lifecycle states for CppGram.
 */

#include <string>
#include <vector>
#include <cstdint>
#include "cppgram/types.hpp"

namespace cppgram {

/**
 * @brief State of an end-to-end encrypted secret chat.
 */
enum class SecretChatState {
    Pending,
    Ready,
    Closed
};

/**
 * @brief Represents an end-to-end encrypted Telegram secret chat session.
 */
struct SecretChat {
    int32_t         id{0};
    UserId          user_id{0};
    SecretChatState state{SecretChatState::Pending};
    bool            is_outbound{false};
    int32_t         ttl{0};
    int32_t         layer{0};
    std::string     key_hash;
};

} // namespace cppgram
