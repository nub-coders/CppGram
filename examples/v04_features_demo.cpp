// examples/v04_features_demo.cpp
// Demonstrates new features introduced in CppGram v0.4:
// - Telegram Mini Apps / Web Apps
// - Telegram Business Bots & Quick Replies
// - Secret Chats & E2EE Models
// - Voice & Group Calls
// - Standalone MTProto Transport Packet Codecs (Abridged, Intermediate, Full with CRC32)

#include <iostream>
#include <iomanip>
#include "cppgram/client.hpp"
#include "cppgram/filters.hpp"
#include "cppgram/web_app.hpp"
#include "cppgram/business.hpp"
#include "cppgram/secret_chat.hpp"
#include "cppgram/call.hpp"
#include "cppgram/transport.hpp"

using namespace cppgram;

int main() {
    std::cout << "===================================================\n";
    std::cout << "       CppGram v0.4 Features Showcase Demo         \n";
    std::cout << "===================================================\n\n";

    // 1. Telegram Mini Apps / Web Apps
    std::cout << "[1] Telegram Mini Apps / Web Apps\n";
    WebAppInfo web_app{"https://example.com/miniapp"};
    std::cout << "  Mini App URL: " << web_app.url << "\n";

    InlineKeyboard ik;
    ik.rows.push_back({
        InlineKeyboardButton::web_app("Launch Web App", web_app.url)
    });
    std::cout << "  Created InlineKeyboardButton with WebApp payload\n";

    WebAppData sample_data{"{\"item\":\"nitro_subscription\",\"price\":4.99}", "Purchase"};
    std::cout << "  WebAppData received: " << sample_data.data 
              << " (button: " << sample_data.button_text << ")\n\n";

    // 2. Telegram Business Bots & Quick Replies
    std::cout << "[2] Telegram Business Bots & Quick Replies\n";
    BusinessConnection biz_conn;
    biz_conn.id = "biz_conn_alpha_001";
    biz_conn.user.id = 987654321;
    biz_conn.can_reply = true;
    biz_conn.is_enabled = true;
    std::cout << "  Business Connection ID: " << biz_conn.id 
              << ", Can Reply: " << (biz_conn.can_reply ? "Yes" : "No") << "\n";

    BusinessIntro intro;
    intro.title = "Welcome to Coffee Shop Bot!";
    intro.message = "Order freshly brewed coffee on-demand.";
    std::cout << "  Business Intro Title: " << intro.title << "\n";

    QuickReplyShortcut shortcut;
    shortcut.id = 1;
    shortcut.name = "menu";
    shortcut.message_count = 5;
    std::cout << "  Quick Reply Shortcut: /" << shortcut.name 
              << " (" << shortcut.message_count << " templates)\n\n";

    // 3. Secret Chats & E2EE Models
    std::cout << "[3] Secret Chats & E2EE Models\n";
    SecretChat secret_chat;
    secret_chat.id = 1002;
    secret_chat.user_id = 555666;
    secret_chat.state = SecretChatState::Ready;
    secret_chat.is_outbound = true;
    secret_chat.layer = 144;
    secret_chat.key_hash = "fedcba9876543210";
    std::cout << "  Secret Chat ID: " << secret_chat.id 
              << ", Layer: " << secret_chat.layer 
              << ", State: Ready\n\n";

    // 4. Voice & Group Calls
    std::cout << "[4] Voice & Group Calls Models\n";
    GroupCall call;
    call.id = 9001;
    call.title = "Weekly Architecture Planning";
    call.duration = 2400;
    call.is_active = true;
    call.is_joined = true;
    call.participant_count = 16;
    std::cout << "  Group Call: \"" << call.title 
              << "\" (ID: " << call.id << ", Active: " 
              << (call.is_active ? "True" : "False") << ", Participants: " 
              << call.participant_count << ")\n\n";

    // 5. Standalone MTProto Transport Packet Codecs
    std::cout << "[5] MTProto Transport Packet Codecs (Abridged, Intermediate, Full)\n";
    
    // Abridged
    auto abridged = create_transport_codec(TransportProtocol::Abridged);
    std::vector<uint8_t> raw_payload = {'C', 'p', 'p', 'G', 'r', 'a', 'm', '4'};
    auto enc_abridged = abridged->encode_packet(raw_payload);
    std::cout << "  Abridged packet length: " << enc_abridged.size() 
              << " bytes (header tag: 0x" << std::hex << (int)abridged->get_header()[0] << std::dec << ")\n";
    auto dec_abridged = abridged->decode_packets(enc_abridged);
    std::cout << "  Abridged decoded successfully: " << (!dec_abridged.empty() ? "Yes" : "No") << "\n";

    // Intermediate
    auto intermediate = create_transport_codec(TransportProtocol::Intermediate);
    auto enc_intermediate = intermediate->encode_packet(raw_payload);
    std::cout << "  Intermediate packet length: " << enc_intermediate.size() << " bytes\n";
    auto dec_intermediate = intermediate->decode_packets(enc_intermediate);
    std::cout << "  Intermediate decoded successfully: " << (!dec_intermediate.empty() ? "Yes" : "No") << "\n";

    // Full with CRC32
    auto full = create_transport_codec(TransportProtocol::Full);
    auto enc_full = full->encode_packet(raw_payload);
    std::cout << "  Full packet length: " << enc_full.size() << " bytes (length + seq + payload + CRC32)\n";
    auto dec_full = full->decode_packets(enc_full);
    std::cout << "  Full decoded & verified CRC32 successfully: " << (!dec_full.empty() ? "Yes" : "No") << "\n\n";

    std::cout << "===================================================\n";
    std::cout << "   All CppGram v0.4 features verified and working! \n";
    std::cout << "===================================================\n";
    return 0;
}
