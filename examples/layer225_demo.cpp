#include "cppgram/tl.hpp"
#include "cppgram/mtproto_client.hpp"
#include "cppgram/types.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace cppgram;

int main() {
    std::cout << "=======================================================\n";
    std::cout << "      CppGram Telegram MTProto API Layer 225 Demo     \n";
    std::cout << "=======================================================\n\n";

    // 1. Layer 225 Protocol Versioning
    std::cout << "[1] Telegram API Layer Alignment:\n";
    std::cout << "  Active Core Schema Layer: " << TELEGRAM_API_LAYER << "\n";
    assert(TELEGRAM_API_LAYER == 225);

    // 2. Layer 225 Constructor Identifiers
    std::cout << "\n[2] Layer 225 Constructor Identifiers:\n";
    std::cout << "  TL_INVOKE_WITH_LAYER:                 0x" << std::hex << TL_INVOKE_WITH_LAYER << std::dec << "\n";
    std::cout << "  TL_INIT_CONNECTION:                   0x" << std::hex << TL_INIT_CONNECTION << std::dec << "\n";
    std::cout << "  TL_MESSAGES_SET_BOT_GUEST_CHAT_RESULT:0x" << std::hex << TL_MESSAGES_SET_BOT_GUEST_CHAT_RESULT << std::dec << "\n";
    std::cout << "  TL_STATS_GET_POLL_STATS:              0x" << std::hex << TL_STATS_GET_POLL_STATS << std::dec << "\n";
    std::cout << "  TL_AICOMPOSE_CREATE_TONE:             0x" << std::hex << TL_AICOMPOSE_CREATE_TONE << std::dec << "\n";
    std::cout << "  TL_AICOMPOSE_GET_TONES:               0x" << std::hex << TL_AICOMPOSE_GET_TONES << std::dec << "\n";

    assert(TL_INVOKE_WITH_LAYER == 0xda9b0d0d);
    assert(TL_INIT_CONNECTION == 0xc1cd5ea9);
    assert(TL_MESSAGES_SET_BOT_GUEST_CHAT_RESULT == 0x052b08db);
    assert(TL_STATS_GET_POLL_STATS == 0xc27dfa68);
    assert(TL_AICOMPOSE_CREATE_TONE == 0x4aa83913);

    // 3. invokeWithLayer#da9b0d0d Envelope Packaging & Unpacking
    std::cout << "\n[3] invokeWithLayer#da9b0d0d Protocol Framing:\n";
    std::vector<uint8_t> inner_rpc = {0x10, 0x20, 0x30, 0x40, 0x50};
    auto layer_envelope = MtprotoClient::build_invoke_with_layer_query(TELEGRAM_API_LAYER, inner_rpc);
    std::cout << "  Serialized Envelope Size: " << layer_envelope.size() << " bytes\n";
    assert(layer_envelope.size() == 8 + inner_rpc.size());

    int32_t parsed_layer = 0;
    std::vector<uint8_t> parsed_inner_rpc;
    TLReader envelope_reader(layer_envelope);
    bool ok_parse = envelope_reader.read_invoke_with_layer(parsed_layer, parsed_inner_rpc);
    assert(ok_parse);
    assert(parsed_layer == 225);
    assert(parsed_inner_rpc == inner_rpc);
    std::cout << "  Decoded Layer: " << parsed_layer << " (Matches expected Layer 225)\n";
    std::cout << "  Payload Integrity: VERIFIED (" << parsed_inner_rpc.size() << " bytes)\n";

    // 4. initConnection Framing
    std::cout << "\n[4] initConnection Layer 225 Bootstrap Framing:\n";
    TLWriter init_w;
    init_w.write_init_connection(
        123456,
        "Server Linux x86_64",
        "Linux 6.6",
        "CppGram 1.1.0 (Layer 225)",
        "en",
        "tdesktop",
        "en",
        inner_rpc
    );
    auto init_packet = init_w.take_data();
    std::cout << "  initConnection Packet Size: " << init_packet.size() << " bytes\n";
    TLReader init_r(init_packet);
    assert(init_r.read_uint32() == TL_INIT_CONNECTION);
    assert(init_r.read_uint32() == 0); // flags
    assert(init_r.read_int32() == 123456); // api_id
    assert(init_r.read_string() == "Server Linux x86_64");

    // 5. Layer 225 Domain Entities (AI Compose Tones, Guest Mode, Poll Stats)
    std::cout << "\n[5] Layer 225 Domain Entities:\n";
    AiComposeTone tone;
    tone.id = 998877;
    tone.title = "Professional & Direct";
    tone.prompt = "Rephrase this message with clear, executive-level brevity.";
    tone.emoji_id = 54321;
    tone.display_author = true;
    std::cout << "  AI Compose Tone: \"" << tone.title << "\" (ID: " << tone.id << ")\n";

    BotGuestChatResult guest_res;
    guest_res.query_id = 123456789LL;
    guest_res.text = "Hello as a guest via bot!";
    guest_res.parse_mode = ParseMode::HTML;
    std::cout << "  Bot Guest Chat Result: \"" << guest_res.text << "\" (Query ID: " << guest_res.query_id << ")\n";

    PollStats poll_stats;
    poll_stats.total_voters = 1420;
    poll_stats.option_voters = {850, 420, 150};
    std::cout << "  Poll Statistics: " << poll_stats.total_voters << " total voters across "
              << poll_stats.option_voters.size() << " options\n";

    std::cout << "\n=======================================================\n";
    std::cout << "   Telegram MTProto API Layer 225 Validated Cleanly!  \n";
    std::cout << "=======================================================\n";
    return 0;
}
