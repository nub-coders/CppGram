# CppGram

A modern, high-performance Telegram client framework for C++20 built on top of TDLib.

CppGram brings the developer experience of Pyrogram and Telethon to the C++ ecosystem while maintaining performance, type safety, and scalability.

---

**Features**

*Telegram Bot API 10.3 Rich Messages & Rich Text*
- Structured multi-block document model (`InputRichMessage`, `RichMessage`, `RichBlock`)
- Compact table formatting with headers and alignments (`RichBlockTable` with `is_compact`)
- Expandable block quotations (`RichBlockExpandableBlockQuotation`)
- Rich message buttons with styles, copy text, and disabled status (`RichBlockButtons`, `RichMessageButton`)
- Embedded document blocks and general file links (`RichBlockDocument` with `tg://document?id=`)
- AI message draft streaming with thinking blocks (`RichBlockThinking`, `Client::sendRichMessageDraft`)
- Granular rich text span entities (`RichText`, `RichTextSpan`, `RichTextStyle`)
- Fluent document construction API (`RichMessageBuilder`)
- Direct client and message reply routing (`Client::sendRichMessage`, `Message::reply(InputRichMessage)`)

*Telegram MTProto API Layer 225 Support*
- Fully aligned with Telegram MTProto Layer 225 specification (`TELEGRAM_API_LAYER = 225`)
- `invokeWithLayer#da9b0d0d` envelope framing and parsing (`TLWriter::write_invoke_with_layer`, `TLReader::read_invoke_with_layer`)
- Layer 225 `initConnection#c1cd5ea9` client initialization framing
- Custom AI Compose Tone domain entities (`AiComposeTone`) and management constructor constants
- Bot Guest Chat queries and responses (`BotGuestChatResult`, `TL_MESSAGES_SET_BOT_GUEST_CHAT_RESULT`)
- Poll analytics and statistics domain entities (`PollStats`, `TL_STATS_GET_POLL_STATS`)

*MTProto Obfuscated Transport & Fake-TLS (v1.1)*
- MTProto Obfuscated2 anti-censorship transport codec (`ObfuscatedCodec`)
- 64-byte randomized handshake header preventing DPI heuristic fingerprinting
- Dual-direction AES-256-CTR frame encryption and decryption
- Fake-TLS 1.3 `ClientHello` frame generator (`FakeTls::create_client_hello`) with SNI domain fronting
- TLS record header and version validation (`FakeTls::is_valid_tls_record`)

*Multi-Account Session Pool Orchestrator (v1.1)*
- Multi-account fleet orchestration (`AccountPool`) for user and bot sessions
- Thread-safe account registration, retrieval, removal, and size tracking
- Atomic round-robin account selection (`get_next_account`) for distributed workload dispatch
- Multi-chat broadcast routine (`AccountPool::broadcast`)

*Real-Time Telemetry & Prometheus Exporter (v1.1)*
- Enterprise metrics collector (`MetricsCollector`) with thread-safe atomic counters
- Tracking messages sent/received, total RPC invocations, errors, and reconnect events
- Real-time active socket connection gauge and average RPC latency tracking
- Standard Prometheus text format exporter (`to_prometheus_format`) for Grafana scraping

*CMake Downstream Packaging & Installation (v1.1)*
- Modern CMake `install()` export rules and package configuration
- Downstream integration via `find_package(CppGram REQUIRED)` and `target_link_libraries(app PRIVATE CppGram::cppgram)`

*Authentication*
- User account login with phone number
- Bot account login with BotFather token
- OTP verification via callback
- Two-factor authentication (2FA)
- Automatic session persistence via TDLib and SQLite storage

*Telegram Mini Apps & Web Apps*
- Full domain models for Web Apps (`WebAppInfo`, `WebAppData`, `SentWebAppMessage`, `WebAppOpenMode`)
- WebApp buttons on Inline and Reply keyboards (`InlineKeyboardButton::web_app`, `ReplyKeyboardButton::web_app`)
- WebApp data filters (`Filters::webAppData()`) and answer queries (`client.answerWebAppQuery`, `client.asyncAnswerWebAppQuery`)

*Telegram Business Bots & Quick Replies*
- Business connection management (`BusinessConnection`, `BusinessIntro`, `BusinessLocation`, `BusinessOpeningHours`)
- Quick reply templates & shortcut models (`QuickReplyShortcut`, `BusinessMessages`)
- Business event routing filters (`Filters::business()`, `Filters::businessConnectionId(id)`)

*Secret Chats & End-to-End Encryption (E2EE)*
- Domain models and state tracking for secret chats (`SecretChat`, `SecretChatState`)
- Secret chat lifecycle management (`createSecretChat`, `closeSecretChat`, `getSecretChat`)
- Coroutine-based secret chat methods (`asyncCreateSecretChat`, `asyncCloseSecretChat`, `asyncGetSecretChat`)

*Voice & Group Calls*
- Group and voice call domain entities (`GroupCall`, `GroupCallParticipant`, `CallProtocol`, `CallServer`, `CallDiscardReason`)
- Call management API (`getGroupCall`, `joinGroupCall`, `leaveGroupCall`)
- Coroutine call actions (`asyncGetGroupCall`, `asyncJoinGroupCall`, `asyncLeaveGroupCall`)

*MTProto 2.0 Cryptographic Engine*
- High-performance AES-256-IGE (`CryptoUtils::aes_ige_encrypt`, `aes_ige_decrypt`) with zero deprecations
- AES-256-CTR streaming cipher primitives (`CryptoUtils::aes_ctr_encrypt`, `aes_ctr_decrypt`)
- MTProto 2.0 Key Derivation Function (`CryptoUtils::kdf_mtproto2`) for client and server key/IV generation
- MTProto 2.0 message key calculation (`CryptoUtils::compute_msg_key`) and AuthKey identifier generation (`CryptoUtils::compute_auth_key_id`)
- SHA-256 and SHA-1 digest utilities

*Datacenter Network & Socket Layer*
- Telegram DataCenter manager (`DatacenterManager`) with production (DC 1..5) and test DC endpoints
- Dynamic DC selection, primary DC switching, and custom DC registration
- Non-blocking TCP socket connection wrapper (`TcpConnection`) with timeout handling and MTProto codec framing

*MTProto Session & State Management*
- Session state tracking (`Session`) with session IDs, server salt, and time synchronization offsets
- Monotonic timestamp-based 64-bit message ID generation (`generate_msg_id`)
- Content-related sequence numbering (`generate_seq_no`)
- Unencrypted and encrypted MTProto message envelope serialization and deserialization

*Binary Type Language (TL) Codec Engine*
- High-performance binary TL serialization and deserialization (`TLWriter`, `TLReader`)
- Native support for `int32`, `int64`, `int128`, `int256`, `double`, `string`, `bytes`, and generic vectors (`vector<T>`)
- Core MTProto constructor constants (`TL_VECTOR`, `TL_PING`, `TL_PONG`, `TL_REQ_PQ_MULTI`, `TL_RES_PQ`, `TL_RPC_RESULT`)

*Native MTProto Client Subsystem*
- Standalone C++20 MTProto network engine (`MtprotoClient`) operating with zero TDLib overhead
- Direct TCP transport stream integration across DC 1 to 5 with automatic ping keep-alives
- Dual backend selector via `ClientConfig` (`BackendType::TDLib` and `BackendType::NativeMTProto`)

*Performance Benchmark Suite*
- Micro-benchmark suite (`benchmarks/engine_bench.cpp`) measuring AES-256-IGE throughput, framing codecs throughput, and TL serialization roundtrips

*Interactive CLI & REPL Framework*
- Terminal command dispatcher (`InteractiveCLI`) with custom command registration
- Built-in commands (`/help`, `/clear`, `/exit`, `/quit`)
- Argument tokenization with quotes support and interactive prompt / banner configuration

*Standalone MTProto Transport Codecs*
- Standalone framing and serialization codecs for MTProto TCP streaming
- `AbridgedCodec` (0xef prefix, compact variable-length framing)
- `IntermediateCodec` (0xeeeeeeee prefix, 4-byte little-endian length)
- `FullCodec` (sequence numbered frames with IEEE 802.3 CRC32 verification)
- Factory initialization via `create_transport_codec(TransportProtocol::...)`

*Modular Plugin Architecture*
- Abstract `IPlugin` lifecycle interface (`on_load`, `on_unload`)
- `PluginManager` for dynamic registration, listing, and unloading of command modules
- Isolated, reusable feature modules loaded directly onto `Client` (`client.loadPlugin(...)`)

*Middleware Pipeline*
- Flexible pre- and post-processing interceptor pipeline via `client.use(...)`
- `MiddlewareContext` supporting custom metadata storage and event inspection
- Early short-circuiting and event cancellation (`ctx.stop_propagation()`)

*Worker Thread Pool & Dispatch Concurrency*
- Built-in `ThreadPool` with thread-safe task queues and task counters
- Configurable worker count via `client.setThreadPoolSize(N)`
- Non-blocking concurrent execution of update handlers and background operations

*Message Threads & Forum Topics*
- Native Telegram message thread models (`MessageThreadInfo`, `ForumTopic`)
- Thread-specific message sending and replying (`msg.reply_in_thread(...)`)
- Querying thread information and message histories (`getMessageThread`, `getMessageThreadHistory`)
- Thread-aware message filters (`Filters::thread(id)`, `Filters::in_thread()`)

*Telegram Stories Models*
- Domain models for Telegram Stories (`StoryItem`, `Stories`)
- Granular audience privacy configuration (`StoryPrivacySettings`, `StoryPrivacy`)

*Messaging & Rich Text Formatting*
- Send, edit, delete, forward messages
- Reply to messages and reply within specific discussion threads
- Parse modes: Markdown, MarkdownV2, HTML, and entity extraction
- Pin / unpin messages
- Reactions (emoji)
- Message search (global and per-chat)

*Scheduled Messages*
- Send scheduled messages at specific dates / timestamps
- Retrieve scheduled message lists per chat
- Send scheduled messages immediately (`sendScheduledMessageNow`)
- Delete scheduled messages

*Media & Profile Management*
- Send photos, videos, documents, audio, voice notes, video notes, animations, stickers
- Media metadata extraction (dimensions, duration, file size, thumbnails)
- Set and delete user profile photos
- Set and delete chat profile photos
- Retrieve user profile photos history
- File download with progress callback

*Rich Messages & Polls*
- Polls (regular and quiz mode)
- Dice
- Contacts
- Locations (including live location)
- Venues

*Inline Keyboards & Callbacks*
- Inline keyboard buttons (callback data, URLs, WebApps, switch inline)
- Reply keyboards with WebApp launching
- Remove keyboard / force reply
- Callback query handling and answering

*Chat Management*
- Create groups, supergroups, channels
- Set title, description, photo, permissions
- Ban, unban, restrict, promote members
- Get member list, count, administrators
- Generate invite links
- Join / leave chats

*Session Storage & Caching*
- Pluggable `ISessionStorage` interface
- Built-in SQLite3 persistent backend (`SqliteSessionStorage`) with WAL mode & transactions
- In-memory session store (`MemorySessionStorage`)
- Peer ID / Username caching

*Native C++20 Coroutines*
- `Task<T>` and `Task<void>` coroutine abstractions
- Asynchronous client methods (`asyncSendMessage`, `asyncGetMe`, `asyncCreateSecretChat`, etc.)
- Non-blocking event handlers with `co_await`

*Developer Experience*
- Pyrogram-inspired API
- Modern C++20
- Composable filter system with `&&`, `||`, `!`
- Entity convenience methods (`msg.reply()`, `msg.reply_in_thread()`, `msg.pin()`, `chat.banMember()`)
- Builder-style keyboard construction
- Cross-platform (Linux, macOS, Windows)

---

**Architecture**

```
Application / Custom Plugins
      │
      ▼
Middleware Pipeline  (Authentication, Rate Limiting, Audit Logging)
      │
      ▼
CppGram API  (Client, Async Coroutines, Session Storage, Thread Management)
      │
      ▼
Event Dispatcher & Thread Pool  (Concurrent Workers, Filters)
      │
      ▼
TDLib Adapter / MTProto Codecs  (Async Request/Response, Packet Codecs)
      │
      ▼
TDLib / TCP Stream
      │
      ▼
Telegram Servers
```

---

**Installation**

*Requirements*
- C++20 compiler (GCC 12+, Clang 15+, MSVC 2022+)
- CMake 3.21+
- OpenSSL
- zlib
- SQLite3
- Threads

*Build Instructions*
```bash
git clone https://github.com/your-org/cppgram.git
cd cppgram
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

TDLib is fetched and built automatically via CMake `FetchContent`.

*Telegram API Credentials*
1. Visit [my.telegram.org](https://my.telegram.org)
2. Log in and create an application
3. Note your `api_id` and `api_hash`

---

**Quick Start**

*Bot Login with Mini Apps & HTML Formatting*
```cpp
#include <cppgram/client.hpp>
#include <cppgram/web_app.hpp>

using namespace cppgram;

int main() {
    Client client(API_ID, "API_HASH");
    client.setDefaultParseMode(ParseMode::HTML);

    client.onMessage(
        Filters::command("app"),
        [](Message msg) {
            InlineKeyboard ik;
            ik.addButton(0, InlineKeyboardButton::web_app(
                "Launch Mini App", "https://webapp.telegram.org"));
            msg.reply("Click below to open our interactive Mini App:", ik);
        }
    );

    client.onMessage(
        Filters::webAppData(),
        [](Message msg) {
            if (msg.web_app_data) {
                msg.reply("Received data from Mini App: " + msg.web_app_data->data);
            }
        }
    );

    client.loginBot("BOT_TOKEN");
    client.run();
}
```

*Telegram Business Bots & Quick Replies*
```cpp
// Filter and handle incoming messages via connected Telegram Business accounts
client.onMessage(
    Filters::business(),
    [](Message msg) {
        if (msg.business_connection_id) {
            msg.reply("Automated Business Reply for connection: " + *msg.business_connection_id);
        }
    }
);
```

*Secret Chats & E2EE Lifecycle*
```cpp
// Create and query an end-to-end encrypted secret chat
SecretChat chat = client.createSecretChat(TARGET_USER_ID);
if (chat.state == SecretChatState::Ready) {
    std::cout << "Secret chat ready on layer " << chat.layer << "\n";
}
```

*Standalone MTProto Transport Codecs*
```cpp
#include <cppgram/transport.hpp>

// Encode and decode packets using the Abridged or Full TCP transport
auto codec = create_transport_codec(TransportProtocol::Abridged);
std::vector<uint8_t> payload = {1, 2, 3, 4, 5};
std::vector<uint8_t> framed_packet = codec->encode_packet(payload);

// Decode stream chunks into complete packets
auto decoded = codec->decode_packets(framed_packet);
```

*Middleware Pipeline*
```cpp
// Attach global logging & authentication middleware
client.use([](MiddlewareContext& ctx) -> bool {
    if (ctx.is_message()) {
        auto& msg = ctx.message();
        if (msg && msg->text.find("BANNED_KEYWORD") != std::string::npos) {
            return false; // Drop update, halt pipeline
        }
    }
    return true; // Continue to next middleware and handlers
});
```

*User Login with SQLite Session Storage*
```cpp
#include <cppgram/client.hpp>
#include <cppgram/storage.hpp>

using namespace cppgram;

int main() {
    auto storage = create_storage("user_session.sqlite3");
    Client client(API_ID, "API_HASH", storage);

    client.login("+1234567890",
        []() -> std::string {
            std::string code;
            std::cout << "Enter OTP: ";
            std::cin >> code;
            return code;
        },
        []() -> std::string {
            std::string pw;
            std::cout << "Enter 2FA password: ";
            std::cin >> pw;
            return pw;
        }
    );

    client.run();
}
```

*C++20 Native Coroutines (`co_await`)*
```cpp
#include <cppgram/client.hpp>
#include <cppgram/coro.hpp>

Task<void> handle_async_ping(Client& client, Message msg) {
    auto me = co_await client.asyncGetMe();
    std::string text = "Pong! Bot: <b>" + me.full_name() + "</b>";
    co_await client.asyncSendMessage(msg.chat_id, text, ParseMode::HTML);
}

// In message handler:
client.onMessage(
    Filters::command("ping"),
    [&client](Message msg) {
        sync_wait(handle_async_ping(client, msg));
    }
);
```

*MTProto 2.0 Cryptography & Session Management*
```cpp
#include <cppgram/crypto.hpp>
#include <cppgram/session.hpp>
#include <cppgram/network.hpp>

// Derive MTProto 2.0 AES-IGE keys
std::vector<uint8_t> auth_key = /* 256 bytes */;
std::vector<uint8_t> msg_key = /* 16 bytes */;
std::vector<uint8_t> aes_key, aes_iv;
CryptoUtils::kdf_mtproto2(auth_key, msg_key, true, aes_key, aes_iv);

// Initialize session and pack encrypted MTProto payload
Session session;
session.set_auth_key(auth_key);
auto frame = session.pack_encrypted_message(payload, true);
```

---

*Binary Type Language (TL) & Native MTProto Client*
```cpp
#include <cppgram/tl.hpp>
#include <cppgram/mtproto_client.hpp>

// Serialize TL objects
TLWriter writer;
writer.write_uint32(TL_REQ_PQ_MULTI);
writer.write_string("CppGram v1.0 Production Client");

// Read and decode TL stream
TLReader reader(writer.data());
uint32_t ctor = reader.read_uint32();
std::string client_name = reader.read_string();

// Connect via standalone Native MTProto Client (zero TDLib overhead)
ClientConfig config;
config.backend = BackendType::NativeMTProto;
config.primary_dc = 2;

MtprotoClient client(config);
client.connect(2, TransportProtocol::Intermediate);
client.ping();
```

*MTProto Obfuscated Transport & Fake-TLS*
```cpp
#include <cppgram/obfuscated.hpp>

// Create an obfuscated transport codec over intermediate framing
auto base_codec = create_transport_codec(TransportProtocol::Intermediate);
ObfuscatedCodec obf_codec(std::move(base_codec), TransportProtocol::Intermediate);

// Retrieve 64-byte anti-DPI handshake header
auto header = obf_codec.get_header();

// Generate Fake-TLS ClientHello with SNI domain fronting
auto client_hello = FakeTls::create_client_hello("cloudflare.com", header);
```

*Multi-Account Pool & Prometheus Telemetry*
```cpp
#include <cppgram/account_pool.hpp>
#include <cppgram/metrics.hpp>

AccountPool pool;
pool.add_account("bot_1", std::make_shared<Client>());
pool.add_account("bot_2", std::make_shared<Client>());

// Round-robin dispatching
auto worker = pool.get_next_account();

MetricsCollector metrics;
metrics.increment_messages_sent(100);
metrics.record_rpc_latency_ms(14.2);

// Export Prometheus exposition format
std::string report = metrics.to_prometheus_format();
```

*Telegram MTProto Layer 225 invokeWithLayer Negotiation*
```cpp
#include <cppgram/tl.hpp>
#include <cppgram/mtproto_client.hpp>

// Package RPC query into an invokeWithLayer#da9b0d0d envelope
std::vector<uint8_t> rpc_query = {'g', 'e', 't', 'P', 'o', 'l', 'l', 'S', 't', 'a', 't', 's'};
auto envelope = MtprotoClient::build_invoke_with_layer_query(TELEGRAM_API_LAYER, rpc_query);

// Parse layer envelope on receiver side
TLReader reader(envelope);
int32_t negotiated_layer = 0;
std::vector<uint8_t> payload;
if (reader.read_invoke_with_layer(negotiated_layer, payload)) {
    // Verified Layer 225 payload
}
```

---

**Project Structure**

```
cppgram/
├── include/cppgram/
│   ├── client.hpp          # Main client class (sync + async coroutines)
│   ├── coro.hpp            # C++20 coroutine Task<T> & sync_wait
│   ├── tl.hpp              # Binary Type Language (TL) Codec (TLWriter, TLReader)
│   ├── mtproto_client.hpp  # Standalone Native MTProto Network Client
│   ├── rich_message.hpp    # Bot API 10.3 Rich Messages, Rich Text & Builder
│   ├── obfuscated.hpp      # MTProto Obfuscated Codec & Fake-TLS Handshake
│   ├── account_pool.hpp    # Multi-account session pool orchestrator
│   ├── metrics.hpp         # Real-time telemetry & Prometheus metrics exporter
│   ├── crypto.hpp          # MTProto 2.0 Crypto (AES-IGE, AES-CTR, KDF, SHA)
│   ├── network.hpp         # DatacenterManager & TcpConnection socket layer
│   ├── session.hpp         # Session state, message IDs & MTProto envelope
│   ├── cli.hpp             # InteractiveCLI command dispatcher & REPL shell
│   ├── web_app.hpp         # Telegram Mini Apps & WebApp models
│   ├── business.hpp        # Telegram Business & Quick Replies models
│   ├── secret_chat.hpp     # Secret chats & E2EE state models
│   ├── call.hpp            # Voice & Group Calls models and protocols
│   ├── transport.hpp       # Standalone MTProto TCP transport codecs
│   ├── plugin.hpp          # IPlugin base & PluginManager lifecycle
│   ├── middleware.hpp      # MiddlewarePipeline & MiddlewareContext
│   ├── thread_pool.hpp     # ThreadPool concurrent worker pool
│   ├── thread.hpp          # MessageThreadInfo & ForumTopic models
│   ├── story.hpp           # Telegram Stories models & privacy settings
│   ├── storage.hpp         # SQLite & In-Memory session storage interface
│   ├── message.hpp         # Message entity & thread reply helpers
│   ├── chat.hpp            # Chat entity + ChatMember
│   ├── user.hpp            # User entity
│   ├── types.hpp           # Core types, ClientConfig, BackendType
│   ├── media.hpp           # Media types (Photo, Video, UserProfilePhotos)
│   ├── keyboard.hpp        # Inline/reply keyboard types (with WebApp support)
│   ├── callback_query.hpp  # Callback query type
│   ├── filters.hpp         # MessageFilter + Filters namespace
│   ├── handlers.hpp        # Handler structs
│   ├── i_backend.hpp       # Backend interface
│   ├── errors.hpp          # Exception hierarchy
│   ├── result.hpp          # Result<T> type
│   └── log.hpp             # Logger
├── src/
│   ├── backend/
│   │   ├── client.cpp      # ClientImpl (TDLib integration & event loop)
│   │   ├── tdlib_adapter.hpp
│   │   ├── td_conversions.hpp
│   │   └── log.cpp
│   └── core/
│       ├── rich_message.cpp # Bot API 10.3 Rich Message builder & block rendering
│       ├── tl.cpp          # Type Language binary serialization implementation
│       ├── mtproto_client.cpp # Native MTProto client engine implementation
│       ├── obfuscated.cpp  # Obfuscated transport codec & Fake-TLS implementation
│       ├── account_pool.cpp # Multi-account pool management & round-robin
│       ├── metrics.cpp     # Telemetry counters & Prometheus format export
│       ├── crypto.cpp      # MTProto 2.0 AES-IGE, AES-CTR & KDF implementation
│       ├── network.cpp     # Datacenter registry & TCP socket connection
│       ├── session.cpp     # Monotonic msg_id, envelope packing & unpacking
│       ├── cli.cpp         # Interactive CLI dispatcher & builtin commands
│       ├── entities.cpp    # Entity convenience methods
│       ├── storage.cpp     # SQLite3 & Memory session storage implementation
│       ├── filters.cpp     # Filter implementations (web_app, business, threads)
│       ├── plugin.cpp      # PluginManager lifecycle implementation
│       └── transport.cpp   # MTProto transport codecs (Abridged, Intermediate, Full)
├── tests/
│   └── phase1_smoke.cpp    # Test suite covering v0.1..v1.1, Layer 225 & Bot API 10.3 (90 tests)
├── benchmarks/
│   ├── CMakeLists.txt      # Benchmarks build definition
│   └── engine_bench.cpp    # Performance benchmarks (AES-IGE, framing codecs, TL)
├── examples/
│   ├── hello.cpp           # Basic bot demonstration
│   ├── v02_features_demo.cpp # Showcase of v0.2 features
│   ├── v03_features_demo.cpp # Showcase of v0.3 features
│   ├── v04_features_demo.cpp # Showcase of v0.4 features (WebApps, Business, Calls, Transport)
│   ├── v05_features_demo.cpp # Showcase of v0.5 features (Crypto, Network, Session, CLI)
│   ├── v10_features_demo.cpp # Showcase of v1.0 features (TL Codecs, Native MTProto Client)
│   ├── v11_features_demo.cpp # Showcase of v1.1 features (Obfuscation, Fake-TLS, Pool, Metrics)
│   ├── layer225_demo.cpp   # Showcase of Telegram API Layer 225 features
│   ├── bot_api_10_3_rich_demo.cpp # Showcase of Telegram Bot API 10.3 Rich Messages
│   └── interactive_cli.cpp # Interactive REPL shell application
└── CMakeLists.txt
```

---

**Roadmap**

*Version 0.1 (Completed)*
- [x] TDLib integration
- [x] Authentication (user + bot)
- [x] Text messaging (send, edit, delete, forward, pin)
- [x] Media messaging (photo, video, document, audio, voice, sticker, animation)
- [x] Rich messages (polls, dice, contacts, locations, venues)
- [x] Inline keyboards + callback queries
- [x] Chat management (create, permissions, ban/restrict/promote)
- [x] Composable filter system
- [x] Event handlers (messages, edits, deletes, callbacks)
- [x] File download
- [x] Message search

*Version 0.2 (Completed)*
- [x] Async coroutines (C++20 `co_await`, `Task<T>`, `sync_wait`)
- [x] Session storage with SQLite (persistent KV & peer cache)
- [x] Profile photo management (user and chat photos)
- [x] Scheduled messages (scheduling, listing, immediate dispatch, deletion)
- [x] Message formatting (Markdown, MarkdownV2, HTML, text entities)

*Version 0.3 (Completed)*
- [x] Modular plugin architecture (`IPlugin`, `PluginManager`)
- [x] Middleware pipeline (`MiddlewarePipeline`, `MiddlewareContext`)
- [x] Message threads & forum topics (`MessageThreadInfo`, `ForumTopic`, `reply_in_thread`, `Filters::thread`)
- [x] Telegram Stories domain models (`StoryItem`, `StoryPrivacySettings`, `Stories`)
- [x] Thread pool concurrency & worker sizing (`ThreadPool`, `setThreadPoolSize`)

*Version 0.4 (Completed)*
- [x] Telegram Mini Apps & Web Apps (`WebAppInfo`, `WebAppData`, `Filters::webAppData`, WebApp buttons)
- [x] Telegram Business Bots & Quick Replies (`BusinessConnection`, `QuickReplyShortcut`, `Filters::business`)
- [x] End-to-end secret chats support (`SecretChat`, `SecretChatState`, `createSecretChat`, `closeSecretChat`)
- [x] Voice and Group calls models & protocols (`GroupCall`, `GroupCallParticipant`, `CallProtocol`)
- [x] Standalone MTProto transport packet codecs (`AbridgedCodec`, `IntermediateCodec`, `FullCodec` with IEEE 802.3 CRC32)

*Version 0.5 (Completed)*
- [x] MTProto 2.0 cryptographic subsystem (`CryptoUtils::aes_ige_encrypt`, `aes_ige_decrypt`, `kdf_mtproto2`, `compute_msg_key`)
- [x] Datacenter Network Management & non-blocking socket stream (`DatacenterManager`, `TcpConnection`)
- [x] MTProto Session state & message ID sequence tracking (`Session`, `pack_encrypted_message`, `unpack_encrypted_message`)
- [x] Interactive CLI REPL shell framework (`InteractiveCLI`, `CommandContext`)

*Version 1.0 (Completed - Production Release)*
- [x] Binary Type Language (TL) Codec Engine (`TLWriter`, `TLReader`)
- [x] Standalone Native MTProto Network Client (`MtprotoClient`, `BackendType::NativeMTProto`)
- [x] Engine performance benchmark suite (`engine_bench`)
- [x] Comprehensive 60-test smoke verification suite (100% passing)
- [x] Production-ready stable release candidate

*Version 1.1 (Enterprise & Cloud Native - Completed)*
- [x] MTProto Obfuscated2 anti-censorship transport codec (`ObfuscatedCodec`)
- [x] Fake-TLS 1.3 handshake frame generator & SNI domain fronting (`FakeTls`)
- [x] Multi-account session pool orchestrator (`AccountPool`)
- [x] Real-time Prometheus metrics exporter (`MetricsCollector`)
- [x] Modern CMake packaging & installation targets (`find_package(CppGram)`)
- [x] Comprehensive 70-test smoke verification suite (100% passing)

*Telegram Layer 225 Upgrade (Completed)*
- [x] Protocol schema alignment to Telegram MTProto Layer 225 (`TELEGRAM_API_LAYER = 225`)
- [x] `invokeWithLayer#da9b0d0d` query encapsulation & deserialization
- [x] `initConnection#c1cd5ea9` Layer 225 bootstrap framing
- [x] AI Compose Tones domain models & constructor constants
- [x] Bot Guest Mode result routing & Poll statistics
- [x] 80-test smoke test suite (100% passing)

---

**Contributing**

Contributions are welcome.

Before contributing:
- Run `clang-format` on your code
- Run `ctest` to verify tests pass

All pull requests must pass CI checks.

---

**License**

MIT License

Copyright (c) 2026 CppGram Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files to deal in the Software
without restriction.

---

**Acknowledgements**

- [Telegram](https://telegram.org)
- [TDLib](https://github.com/tdlib/td)
- [Pyrogram](https://github.com/pyrogram/pyrogram)
- [Telethon](https://github.com/LonamiWebs/Telethon)
