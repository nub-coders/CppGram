#pragma once

/**
 * @file call.hpp
 * @brief Voice and Video Call domain models, protocol abstractions, and Group Call entities.
 */

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include "cppgram/types.hpp"

namespace cppgram {

/**
 * @brief Reason why a phone/voice call was discarded or disconnected.
 */
enum class CallDiscardReason {
    Missed,
    Disconnect,
    Hangup,
    Busy
};

/**
 * @brief Network protocol capabilities supported by the client for VoIP WebRTC calls.
 */
struct CallProtocol {
    bool udp_p2p{true};
    bool udp_reflector{true};
    int32_t min_layer{65};
    int32_t max_layer{92};
    std::vector<std::string> library_versions;
};

/**
 * @brief Represents a VoIP relay / reflector server endpoint.
 */
struct CallServer {
    int64_t     id{0};
    std::string ip;
    std::string ipv6;
    int32_t     port{0};
    std::string type;
    std::string peer_tag;
};

/**
 * @brief Represents a participant in an ongoing Telegram Group Voice/Video Chat.
 */
struct GroupCallParticipant {
    UserId  user_id{0};
    bool    is_muted{false};
    bool    is_speaking{false};
    bool    is_hand_raised{false};
    bool    can_unmute_self{true};
    int32_t volume_level{10000};
    std::string order;
};

/**
 * @brief Represents an ongoing or scheduled Telegram Group Voice/Video Call in a chat.
 */
struct GroupCall {
    int32_t     id{0};
    std::string title;
    int32_t     duration{0};
    bool        is_active{false};
    bool        is_joined{false};
    bool        need_rejoin{false};
    bool        can_enable_video{false};
    int32_t     participant_count{0};
    bool        loaded_all_participants{false};
    std::vector<GroupCallParticipant> participants;
};

} // namespace cppgram
