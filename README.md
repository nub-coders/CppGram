# CppGram

A modern, high-performance Telegram client framework for C++20 built on top of TDLib.

CppGram brings the developer experience of Pyrogram and Telethon to the C++ ecosystem while maintaining performance, type safety, and scalability.

## Features

### Authentication
- User account login with phone number
- Bot account login with BotFather token
- OTP verification via callback
- Two-factor authentication (2FA)
- Automatic session persistence via TDLib

### Messaging
- Send, edit, delete, forward messages
- Reply to messages
- Pin / unpin messages
- Reactions (emoji)
- Message search (global and per-chat)

### Media
- Send photos, videos, documents, audio, voice notes, video notes, animations, stickers
- Media metadata extraction (dimensions, duration, file size, thumbnails)
- File download with progress callback

### Rich Messages
- Polls (regular and quiz mode)
- Dice
- Contacts
- Locations (including live location)
- Venues

### Inline Keyboards & Callbacks
- Inline keyboard buttons (callback data, URLs, switch inline)
- Reply keyboards
- Remove keyboard / force reply
- Callback query handling and answering

### Chat Management
- Create groups, supergroups, channels
- Set title, description, photo, permissions
- Ban, unban, restrict, promote members
- Get member list, count, administrators
- Generate invite links
- Join / leave chats

### User Operations
- Get user info and profile
- Block / unblock users

### Events
- New messages (`onMessage`)
- Edited messages (`onEditedMessage`)
- Deleted messages (`onDeletedMessages`)
- Callback queries (`onCallbackQuery`)

### Filters
- Chat type: `privateChat()`, `group()`, `channel()`
- Content: `text()`, `media()`, `photo()`, `video()`, `document()`, `audio()`, `voice()`, `sticker()`, `animation()`, `pollMsg()`, `locationMsg()`, `contactMsg()`, `dice()`
- Message state: `incoming()`, `outgoing()`, `reply()`, `forwarded()`, `bot()`
- Targeting: `command("name")`, `chatId(id)`, `userId(id)`, `regex("pattern")`
- **Composable**: `Filters::text() && Filters::privateChat() && !Filters::bot()`

### Developer Experience
- Pyrogram-inspired API
- Modern C++20
- Composable filter system with `&&`, `||`, `!`
- Entity methods (`msg.reply()`, `msg.pin()`, `chat.banMember()`)
- Builder-style keyboard construction
- Event-driven architecture
- Cross-platform (Linux, macOS, Windows)

## Architecture

```
Application
      │
      ▼
CppGram API  (Client)
      │
      ▼
Event Dispatcher  (handlers, filters)
      │
      ▼
TDLib Adapter  (async request/response)
      │
      ▼
TDLib
      │
      ▼
Telegram Servers
```

## Installation

### Requirements
- C++20 compiler (GCC 12+, Clang 15+, MSVC 2022+)
- CMake 3.21+
- OpenSSL
- zlib
- SQLite3 (optional)

### Build
```bash
git clone https://github.com/your-org/cppgram.git
cd cppgram
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

TDLib is fetched automatically via CMake FetchContent (pinned to a specific version).

### Telegram API Credentials

1. Visit [my.telegram.org](https://my.telegram.org)
2. Log in and create an application
3. Note your `api_id` and `api_hash`

## Quick Start

### Bot Login
```cpp
#include <cppgram/client.hpp>

using namespace cppgram;

int main() {
    Client client(API_ID, "API_HASH");
    client.loginBot("BOT_TOKEN");
    client.run();
}
```

### User Login
```cpp
Client client(API_ID, "API_HASH");

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
```

### Message Handlers
```cpp
// Handle all text messages
client.onMessage(
    Filters::text(),
    [](Message msg) {
        std::cout << msg.sender.username << ": " << msg.text << "\n";
    }
);

// Handle commands
client.onMessage(
    Filters::command("ping"),
    [](Message msg) {
        msg.reply("pong");
    }
);

// Compose filters
client.onMessage(
    Filters::text() && Filters::privateChat() && !Filters::bot(),
    [](Message msg) {
        msg.reply("Hello, " + msg.sender.first_name + "!");
    }
);
```

### Inline Keyboards
```cpp
client.onMessage(
    Filters::command("menu"),
    [](Message msg) {
        InlineKeyboard kb;
        kb.addButton(0, InlineKeyboardButton::callback("Option A", "a"));
        kb.addButton(0, InlineKeyboardButton::callback("Option B", "b"));
        kb.addButton(1, InlineKeyboardButton::link("Website", "https://example.com"));
        msg.reply("Choose:", kb);
    }
);

client.onCallbackQuery([](CallbackQuery q) {
    q.answer("You chose: " + q.data);
});
```

### Send Media
```cpp
client.sendPhoto(chat_id, InputFile::local("image.jpg"), "My photo");
client.sendDocument(chat_id, InputFile::local("report.pdf"), "Q4 Report");
client.sendVideo(chat_id, InputFile::local("clip.mp4"));
client.sendSticker(chat_id, InputFile::local("sticker.webp"));
```

### Polls
```cpp
client.sendPoll(chat_id, "Favorite language?", {"C++", "Rust", "Python"});

// Quiz mode
PollConfig quiz;
quiz.type = PollType::Quiz;
quiz.correct_option_id = 0;
quiz.explanation = "C++ is the best!";
client.sendPoll(chat_id, "What is CppGram written in?", {"C++", "Python"}, quiz);
```

### Chat Management
```cpp
auto chat = client.getChat(chat_id);

chat.setTitle("New Title");
chat.setDescription("Updated description");
chat.banMember(user_id);

ChatAdminRights rights;
rights.can_delete_messages = true;
rights.can_pin_messages = true;
chat.promoteMember(user_id, rights);
```

## Project Structure
```
cppgram/
├── include/cppgram/
│   ├── client.hpp          # Main client class
│   ├── message.hpp         # Message entity
│   ├── chat.hpp            # Chat entity + ChatMember
│   ├── user.hpp            # User entity
│   ├── types.hpp           # Core types, enums, structs
│   ├── media.hpp           # Media types (Photo, Video, etc.)
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
│   │   ├── client.cpp      # ClientImpl (TDLib integration)
│   │   ├── tdlib_adapter.hpp
│   │   ├── td_conversions.hpp
│   │   └── log.cpp
│   └── core/
│       ├── entities.cpp    # Entity methods
│       └── filters.cpp     # Filter implementations
├── tests/
├── examples/
└── CMakeLists.txt
```

## Roadmap

### Version 0.1 (current)
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

### Version 0.2
- [ ] Async coroutines (C++20 co_await)
- [ ] Session storage with SQLite
- [ ] Profile photo management
- [ ] Scheduled messages
- [ ] Message formatting (bold, italic, code, links)

### Version 0.3
- [ ] Plugin system
- [ ] Forum/topic support
- [ ] Stories
- [ ] Performance optimizations

### Version 1.0
- [ ] Stable API
- [ ] Full documentation
- [ ] Production-ready release

## Contributing

Contributions are welcome.

Before contributing:
- Run `clang-format` on your code
- Run `ctest` to verify tests pass

All pull requests must pass CI checks.

## License

MIT License

Copyright (c) 2026 CppGram Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files to deal in the Software
without restriction.

## Acknowledgements
- [Telegram](https://telegram.org)
- [TDLib](https://github.com/tdlib/td)
- [Pyrogram](https://github.com/pyrogram/pyrogram)
- [Telethon](https://github.com/LonamiWebs/Telethon)
