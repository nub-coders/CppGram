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

*Messaging & Rich Text Formatting*
- Send, edit, delete, forward messages
- Reply to messages
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
- Asynchronous client methods (`asyncSendMessage`, `asyncGetMe`, `asyncGetUser`, etc.)
- Non-blocking event handlers with `co_await`

*Developer Experience*
- Pyrogram-inspired API
- Modern C++20
- Composable filter system with `&&`, `||`, `!`
- Entity convenience methods (`msg.reply()`, `msg.pin()`, `chat.banMember()`)
- Builder-style keyboard construction
- Cross-platform (Linux, macOS, Windows)

---

**Architecture**

```
Application
      │
      ▼
CppGram API  (Client, Async Coroutines, Session Storage)
      │
      ▼
Event Dispatcher  (handlers, filters)
      │
      ▼
TDLib Adapter  (async request/response, conversions)
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

*Bot Login with HTML Formatting*
```cpp
#include <cppgram/client.hpp>

using namespace cppgram;

int main() {
    Client client(API_ID, "API_HASH");
    client.setDefaultParseMode(ParseMode::HTML);
    client.loginBot("BOT_TOKEN");
    
    client.onMessage(
        Filters::command("start"),
        [](Message msg) {
            msg.reply("<b>Welcome to CppGram!</b>\n<i>Fast and type-safe Telegram bots in C++20.</i>");
        }
    );
    
    client.run();
}
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
│   ├── storage.hpp         # SQLite & In-Memory session storage interface
│   ├── message.hpp         # Message entity
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
│       └── filters.cpp     # Filter implementations
├── tests/
│   └── phase1_smoke.cpp    # Full test suite covering v0.1 & v0.2 features
├── examples/
│   ├── hello.cpp           # Basic bot demonstration
│   └── v02_features_demo.cpp # Showcase of all v0.2 features
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

*Version 0.3 (Planned)*
- [ ] Plugin system
- [ ] Forum/topic support
- [ ] Stories
- [ ] Performance optimizations

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
