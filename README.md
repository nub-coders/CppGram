CppGram

A modern, high-performance Telegram MTProto client framework for C++20 built on top of TDLib.

CppGram aims to bring the developer experience of Pyrogram and Telethon to the C++ ecosystem while maintaining the performance, type safety, and scalability of modern C++.

Features
Authentication
User account login
Bot account login
OTP verification
Two-factor authentication (2FA)
Persistent sessions
Automatic session restoration
Messaging
Send messages
Edit messages
Delete messages
Reply to messages
Forward messages
Reactions
Chats
Private chats
Groups
Supergroups
Channels
Media
Photos
Videos
Documents
Audio
Voice messages
Animations
File downloads
Events
New messages
Edited messages
Deleted messages
Reactions
Member updates
Chat updates
Developer Experience
Pyrogram-inspired API
Modern C++20
Async support
Event-driven architecture
Cross-platform
Plugin system
Comprehensive documentation
Why CppGram?

Most Telegram client libraries are written in Python, JavaScript, or Go.

CppGram provides:

Native performance
Low memory usage
Type safety
Easy deployment
Modern C++ API
Full Telegram account support

Inspired by:

Pyrogram
Telethon
TDLib
Architecture
Application
      │
      ▼
CppGram API
      │
      ▼
Event Dispatcher
      │
      ▼
TDLib Adapter
      │
      ▼
TDLib
      │
      ▼
Telegram Servers

CppGram abstracts TDLib into a clean, user-friendly API while maintaining access to Telegram's full capabilities.

Installation
Requirements
C++20 compiler
CMake 3.25+
TDLib
SQLite3
OpenSSL
Clone Repository
git clone https://github.com/your-org/cppgram.git
cd cppgram
Build
mkdir build
cd build

cmake ..
cmake --build . --config Release
Telegram API Credentials

Before using CppGram, create Telegram API credentials.

Visit:

my.telegram.org

Log in with your Telegram account.
Create an application.
Obtain:
api_id
api_hash
Quick Start
User Login
#include <cppgram/client.hpp>

using namespace cppgram;

int main()
{
    Client client(
        API_ID,
        "API_HASH"
    );

    client.login("+1234567890");

    client.run();
}
Bot Login
#include <cppgram/client.hpp>

using namespace cppgram;

int main()
{
    Client client;

    client.loginBot(
        "BOT_TOKEN"
    );

    client.run();
}
Send a Message
Client client(api_id, api_hash);

client.login(phone);

client.sendMessage(
    "username",
    "Hello from CppGram!"
);
Message Handlers
client.onMessage(
    [](Message msg)
    {
        std::cout
            << msg.sender.username
            << ": "
            << msg.text
            << std::endl;
    }
);
Command Filters
client.onMessage(
    Filters::command("ping"),
    [](Message msg)
    {
        msg.reply("pong");
    }
);
Chat Filters
client.onMessage(
    Filters::privateChat(),
    [](Message msg)
    {
        std::cout
            << "Private message received"
            << std::endl;
    }
);
Media Upload
client.sendPhoto(
    chat_id,
    "image.jpg",
    "My Photo"
);
Media Download
client.downloadFile(
    file_id,
    "./downloads"
);
Async API
auto message =
    co_await client.sendMessage(
        chat_id,
        "Hello Async World"
    );
Session Storage

Sessions are automatically persisted.

Stored data includes:

Session ID
User ID
Authorization State
Database Path
Encryption Key
Update State

Users remain logged in across application restarts.

Example Echo Bot
#include <cppgram/client.hpp>

using namespace cppgram;

int main()
{
    Client client(
        API_ID,
        API_HASH
    );

    client.login(
        "+1234567890"
    );

    client.onMessage(
        [](Message msg)
        {
            msg.reply(msg.text);
        }
    );

    client.run();
}
Project Structure
cppgram/

├── include/
│   └── cppgram/
│       ├── client.hpp
│       ├── message.hpp
│       ├── chat.hpp
│       ├── user.hpp
│       ├── filters.hpp
│       ├── handlers.hpp
│       ├── session.hpp
│       ├── errors.hpp
│       └── types.hpp
│
├── src/
│   ├── auth/
│   ├── client/
│   ├── tdlib/
│   ├── updates/
│   ├── handlers/
│   ├── storage/
│   ├── entities/
│   └── utils/
│
├── tests/
├── examples/
├── docs/
├── benchmarks/
└── CMakeLists.txt
Roadmap
Version 0.1
TDLib integration
Authentication
Messaging
Event handling
Session persistence
Version 0.2
Media support
Reactions
Chat management
Improved filters
Version 0.3
Async coroutines
Plugin system
Performance optimizations
Version 1.0
Stable API
Full documentation
Production-ready release
Performance Goals
Metric	Target
Startup Time	< 1s
Memory Usage	< 50 MB
Update Latency	< 10 ms
Concurrent Handlers	1000+
Message Throughput	10,000+/min
Contributing

Contributions are welcome.

Areas of interest:

TDLib integrations
Performance improvements
Documentation
Testing
Platform support

Before contributing:

clang-format
ctest

All pull requests must pass CI checks.

License

MIT License

Copyright (c) 2026 CppGram Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files to deal in the Software without restriction.

Acknowledgements
Telegram
TDLib
Pyrogram
Telethon

Built with ❤️ for modern C++ developers.
