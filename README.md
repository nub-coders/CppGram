# CppGram

A modern, high-performance Telegram client framework for C++20 built on top of TDLib.

CppGram brings the developer experience of Pyrogram and Telethon to the C++ ecosystem while maintaining performance, type safety, and scalability.

---

**Features**

*Authentication*
- User account login with phone number
- Bot account login with BotFather token
- OTP verification via callback
- Two-factor authentication (2FA)
- Automatic session persistence via TDLib and SQLite storage

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
- Inline keyboard buttons (callback data, URLs, switch inline)
- Reply keyboards
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
- Asynchronous client methods (`asyncSendMessage`, `asyncGetMe`, `asyncGetUser`, `asyncGetMessageThread`, etc.)
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
TDLib Adapter  (Async Request/Response, Conversions)
      │
      ▼
TDLib
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

*Bot Login with HTML Formatting & Plugins*
```cpp
#include <cppgram/client.hpp>
#include <cppgram/plugin.hpp>

using namespace cppgram;

class GreeterPlugin : public IPlugin {
public:
    std::string name() const override { return "Greeter"; }
    
    void on_load(Client& client) override {
        client.onMessage(
            Filters::command("start"),
            [](Message msg) {
                msg.reply("<b>Welcome to CppGram!</b>\n<i>Fast, type-safe Telegram bots in C++20.</i>");
            }
        );
    }
};

int main() {
    Client client(API_ID, "API_HASH");
    client.setDefaultParseMode(ParseMode::HTML);
    client.setThreadPoolSize(4); // 4 concurrent worker threads

    client.loadPlugin(std::make_shared<GreeterPlugin>());
    client.loginBot("BOT_TOKEN");
    
    client.run();
}
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

*Message Threads & Forum Topics*
```cpp
// Reply within a specific thread / forum topic
client.onMessage(
    Filters::in_thread(),
    [](Message msg) {
        msg.reply_in_thread("Received message in topic #" + std::to_string(*msg.message_thread_id));
    }
);
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

*Scheduled Messages*
```cpp
auto schedule_time = std::chrono::system_clock::now() + std::chrono::hours(2);

SendMessageOptions options;
options.parse_mode = ParseMode::Markdown;
options.schedule_date = schedule_time;

client.sendMessage(chat_id, "*Reminder:* Team meeting in 10 minutes!", options);
```

*Inline Keyboards & Callback Queries*
```cpp
client.onMessage(
    Filters::command("menu"),
    [](Message msg) {
        InlineKeyboard kb;
        kb.addButton(0, InlineKeyboardButton::callback("Option A", "action_a"));
        kb.addButton(0, InlineKeyboardButton::callback("Option B", "action_b"));
        kb.addButton(1, InlineKeyboardButton::link("GitHub", "https://github.com"));
        msg.reply("Choose an option:", kb);
    }
);

client.onCallbackQuery([](CallbackQuery q) {
    q.answer("Selected: " + q.data);
});
```

*Media & Profile Photo Updates*
```cpp
// Send media
client.sendPhoto(chat_id, InputFile::local("photo.jpg"), "Sample caption");

// Update bot profile photo
client.setProfilePhoto(InputFile::local("avatar.png"));
```

---

**Project Structure**

```
cppgram/
├── include/cppgram/
│   ├── client.hpp          # Main client class (sync + async coroutines)
│   ├── coro.hpp            # C++20 coroutine Task<T> & sync_wait
│   ├── plugin.hpp          # IPlugin base & PluginManager lifecycle
│   ├── middleware.hpp      # MiddlewarePipeline & MiddlewareContext
│   ├── thread_pool.hpp     # ThreadPool concurrent worker pool
│   ├── thread.hpp          # MessageThreadInfo & ForumTopic models
│   ├── story.hpp           # Telegram Stories models & privacy settings
│   ├── storage.hpp         # SQLite & In-Memory session storage interface
│   ├── message.hpp         # Message entity & thread reply helpers
│   ├── chat.hpp            # Chat entity + ChatMember
│   ├── user.hpp            # User entity
│   ├── types.hpp           # Core types, ParseMode, FormattedText, SendMessageOptions
│   ├── media.hpp           # Media types (Photo, Video, UserProfilePhotos)
│   ├── keyboard.hpp        # Inline/reply keyboard types
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
│       ├── entities.cpp    # Entity convenience methods
│       ├── storage.cpp     # SQLite3 & Memory session storage implementation
│       ├── filters.cpp     # Filter implementations
│       └── plugin.cpp      # PluginManager lifecycle implementation
├── tests/
│   └── phase1_smoke.cpp    # Test suite covering v0.1, v0.2, and v0.3 features
├── examples/
│   ├── hello.cpp           # Basic bot demonstration
│   ├── v02_features_demo.cpp # Showcase of v0.2 features
│   └── v03_features_demo.cpp # Showcase of v0.3 features (Plugins, Middleware, Threads, Pool)
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

*Version 0.4 (Planned)*
- [ ] Direct MTProto transport layer (TDLib-free standalone backend)
- [ ] End-to-end secret chats support
- [ ] Custom business bots and Telegram Mini App bot integration
- [ ] Voice and Video calls WebRTC bindings

*Version 1.0*
- [ ] Stable API
- [ ] Full documentation
- [ ] Production-ready release

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
