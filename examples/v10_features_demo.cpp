#include "cppgram/tl.hpp"
#include "cppgram/mtproto_client.hpp"
#include "cppgram/crypto.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace cppgram;

int main() {
    std::cout << "=======================================================\n";
    std::cout << "      CppGram Version 1.0 Production Showcase Demo     \n";
    std::cout << "=======================================================\n\n";

    // 1. Binary Type Language (TL) Codec
    std::cout << "[1] Binary Type Language (TL) Serialization & Deserialization\n";
    TLWriter writer;
    writer.write_uint32(TL_REQ_PQ_MULTI);
    std::array<uint8_t, 16> nonce = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    writer.write_int128(nonce);
    writer.write_string("CppGram v1.0 Production Client");
    writer.write_double(1.6180339887);

    // Vector writing
    writer.write_vector_header(3);
    writer.write_int32(100);
    writer.write_int32(200);
    writer.write_int32(300);

    std::cout << "  Serialized TL Packet Size: " << writer.size() << " bytes (aligned to 4 bytes: "
              << (writer.size() % 4 == 0 ? "YES" : "NO") << ")\n";

    TLReader reader(writer.data());
    uint32_t ctor = reader.read_uint32();
    assert(ctor == TL_REQ_PQ_MULTI);
    auto read_nonce = reader.read_int128();
    assert(read_nonce == nonce);
    std::string client_str = reader.read_string();
    assert(client_str == "CppGram v1.0 Production Client");
    double golden = reader.read_double();
    assert(golden > 1.61 && golden < 1.62);

    uint32_t vec_len = reader.read_vector_header();
    assert(vec_len == 3);
    int32_t v1 = reader.read_int32();
    int32_t v2 = reader.read_int32();
    int32_t v3 = reader.read_int32();
    assert(v1 == 100 && v2 == 200 && v3 == 300);

    std::cout << "  Decoded Constructor: 0x" << std::hex << ctor << std::dec << "\n";
    std::cout << "  Decoded Client String: \"" << client_str << "\"\n";
    std::cout << "  Decoded Vector Elements: [" << v1 << ", " << v2 << ", " << v3 << "]\n";
    std::cout << "  TL roundtrip validated successfully.\n\n";

    // 2. Native MTProto Client Engine
    std::cout << "[2] Native MTProto Client Engine Configuration\n";
    ClientConfig cfg;
    cfg.api_id = 12345;
    cfg.api_hash = "abcdef0123456789";
    cfg.backend = BackendType::NativeMTProto;
    cfg.primary_dc = 2;

    MtprotoClient mtproto(cfg);
    std::cout << "  Active Backend: NativeMTProto (Pure C++20)\n";
    std::cout << "  Configured Primary DC: " << mtproto.get_active_dc_id() << "\n";
    std::cout << "  Transport Protocol: Intermediate\n";

    // Generate ping query
    int64_t ping_id = 9988776655443322LL;
    auto ping_pkt = MtprotoClient::build_ping_query(ping_id);
    std::cout << "  Built Ping Query Size: " << ping_pkt.size() << " bytes\n";

    // Unpack unencrypted message envelope
    auto unenc_envelope = mtproto.get_session().pack_unencrypted_message(ping_pkt);
    int64_t parsed_msg_id = 0;
    std::vector<uint8_t> unpacked_payload;
    bool ok_unenc = Session::unpack_unencrypted_message(unenc_envelope, parsed_msg_id, unpacked_payload);
    assert(ok_unenc && unpacked_payload == ping_pkt);
    std::cout << "  Unencrypted MTProto Envelope Verified (Msg ID: " << parsed_msg_id << ")\n";

    // 3. Encrypted MTProto 2.0 Session Frame
    std::vector<uint8_t> auth_key(256);
    for (size_t i = 0; i < 256; ++i) {
        auth_key[i] = static_cast<uint8_t>((i * 19 + 5) & 0xFF);
    }
    mtproto.get_session().set_auth_key(auth_key);
    auto enc_envelope = mtproto.get_session().pack_encrypted_message(ping_pkt, true);
    std::cout << "  Encrypted MTProto 2.0 Frame Size: " << enc_envelope.size() << " bytes\n";

    Session verifier;
    verifier.set_auth_key(auth_key);
    int64_t rx_msg_id = 0;
    int32_t rx_seq = 0;
    std::vector<uint8_t> rx_payload;
    bool ok_enc = verifier.unpack_encrypted_message(enc_envelope, rx_msg_id, rx_seq, rx_payload);
    assert(ok_enc && rx_payload == ping_pkt);
    std::cout << "  Encrypted Frame Decrypted & Auth Verified (Msg ID: " << rx_msg_id << ", Seq: " << rx_seq << ")\n";

    std::cout << "\n=======================================================\n";
    std::cout << "      Version 1.0 Production Capabilities Ready!       \n";
    std::cout << "=======================================================\n";
    return 0;
}
