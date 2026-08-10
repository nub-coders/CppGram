// examples/v02_features_demo.cpp — CppGram Version 0.2 Features Demonstration.
// Demonstrates:
// 1. Text formatting (Markdown, MarkdownV2, HTML) and Text Entities
// 2. Scheduled Messages (sending, listing, triggering now, deleting)
// 3. Profile Photo Management (user and chat photo updates, retrieving profile photos)
// 4. Persistent Session Storage with SQLite & Peer Caching
// 5. Native C++20 Coroutines (`co_await`, `Task<T>`, async Client API)

#include "cppgram/client.hpp"
#include "cppgram/filters.hpp"
#include "cppgram/storage.hpp"
#include "cppgram/coro.hpp"
#include "cppgram/log.hpp"
#include <cstdlib>
#include <iostream>

#if __has_include(<coroutine>)
// Example coroutine handler using C++20 co_await
cppgram::Task<void> handle_async_ping(cppgram::Client& client, cppgram::Message msg) {
    std::cout << "[Async Handler] Handling /asyncping from chat " << msg.chat_id << "\n";
    
    // Retrieve bot info asynchronously
    auto me = co_await client.asyncGetMe();
    
    // Format response using HTML
    std::string response = "<b>Async Pong!</b>\n"
                           "Bot: <code>" + me.full_name() + "</code> (@" + me.username + ")\n"
                           "<i>C++20 Coroutines in action!</i>";
    
    cppgram::SendMessageOptions options;
    options.parse_mode = cppgram::ParseMode::HTML;
    
    co_await client.asyncSendMessage(msg.chat_id, response, options);
}
#endif

int main() {
    using namespace cppgram;
    Logger::instance().set_level(LogLevel::Info);

    const char* id_str = std::getenv("CPPGRAM_API_ID");
    const char* hash   = std::getenv("CPPGRAM_API_HASH");
    const char* token  = std::getenv("CPPGRAM_BOT_TOKEN");

    if (!id_str || !hash || !token) {
        std::cout << "Usage: Set CPPGRAM_API_ID, CPPGRAM_API_HASH, CPPGRAM_BOT_TOKEN environment variables.\n";
        std::cout << "Running local storage and mock entity verification demo...\n\n";

        // 1. Demonstrate SQLite Session Storage
        std::cout << "--- 1. SQLite Session Storage Demo ---\n";
        auto storage = create_storage("bot_session.sqlite3");
        storage->set_value("version", "0.2.0");
        storage->set_value("last_sync", "2026-09-04T12:00:00Z");
        
        PeerInfo bot_peer{777000, 1, "123456789", "TelegramNotifications", "", ""};
        storage->save_peer(bot_peer);

        auto val = storage->get_value("version");
        if (val) std::cout << "Stored version: " << *val << "\n";

        auto p = storage->get_peer_by_username("TelegramNotifications");
        if (p) std::cout << "Found cached peer ID: " << p->id << " (Type: " << p->type << ")\n";

        // 2. Demonstrate Coroutine Task Execution
#if __has_include(<coroutine>)
        std::cout << "\n--- 2. C++20 Coroutine Task Demo ---\n";
        auto add_async = [](int x, int y) -> Task<int> {
            co_return x + y;
        };
        auto task = [](auto fn) -> Task<int> {
            int a = co_await fn(15, 27);
            co_return a;
        }(add_async);

        int result = sync_wait(std::move(task));
        std::cout << "Coroutine calculation result: 15 + 27 = " << result << "\n";
#endif

        // 3. Demonstrate FormattedText & SendMessageOptions
        std::cout << "\n--- 3. FormattedText & Options Demo ---\n";
        SendMessageOptions opt;
        opt.parse_mode = ParseMode::MarkdownV2;
        opt.schedule_date = std::chrono::system_clock::from_time_t(1750000000);
        opt.protect_content = true;
        std::cout << "Configured SendMessageOptions with Schedule Date: " 
                  << std::chrono::system_clock::to_time_t(*opt.schedule_date) << ", Protect Content: " 
                  << (opt.protect_content ? "true" : "false") << "\n";

        std::cout << "\nTo run live against Telegram, provide API credentials.\n";
        return 0;
    }

    // Initialize client with SQLite persistent session storage
    auto storage = create_storage("bot_session.sqlite3");
    Client client(std::atoi(id_str), hash, storage);
    
    // Set default parse mode for all messages
    client.setDefaultParseMode(ParseMode::HTML);

    client.loginBot(token);
    auto me = client.getMe();
    std::cout << "Logged in as: " << me.full_name() << " (@" << me.username << ")\n";

    // 1. Text formatting demo (/format)
    client.onMessage(
        Filters::command("format"),
        [&client](Message msg) {
            std::string text = "<b>Bold Text</b>, <i>Italic Text</i>, <code>Inline Code</code>\n"
                               "<pre><code class=\"language-cpp\">#include &lt;cppgram/client.hpp&gt;\n"
                               "client.sendMessage(chat_id, \"Hello!\");</code></pre>\n"
                               "<a href=\"https://github.com\">CppGram Repository</a>";
            msg.reply(text, ParseMode::HTML);
        }
    );

    // 2. Scheduled message demo (/schedule)
    client.onMessage(
        Filters::command("schedule"),
        [&client](Message msg) {
            auto in_60_secs = std::chrono::system_clock::now() + std::chrono::seconds(60);
            SendMessageOptions opt;
            opt.parse_mode = ParseMode::Markdown;
            opt.schedule_date = in_60_secs;

            client.sendMessage(
                msg.chat_id,
                "*Reminder:* This message was scheduled 60 seconds ago!",
                opt
            );
            msg.reply("Scheduled a message for you in 60 seconds!");
        }
    );

    // 3. View scheduled messages (/listscheduled)
    client.onMessage(
        Filters::command("listscheduled"),
        [&client](Message msg) {
            auto scheduled = client.getScheduledMessages(msg.chat_id);
            msg.reply("You have " + std::to_string(scheduled.size()) + " scheduled message(s) in this chat.");
        }
    );

    // 4. Async coroutine ping demo (/asyncping)
#if __has_include(<coroutine>)
    client.onMessage(
        Filters::command("asyncping"),
        [&client](Message msg) {
            sync_wait(handle_async_ping(client, msg));
        }
    );
#endif

    // 5. Profile photo management demo (/setmyphoto <file_path>)
    client.onMessage(
        Filters::command("setmyphoto"),
        [&client](Message msg) {
            if (msg.text.size() > 12) {
                std::string path = msg.text.substr(12);
                try {
                    client.setProfilePhoto(InputFile::local(path));
                    msg.reply("Profile photo updated successfully!");
                } catch (const std::exception& e) {
                    msg.reply("Failed to update photo: " + std::string(e.what()));
                }
            } else {
                msg.reply("Usage: /setmyphoto /path/to/avatar.jpg");
            }
        }
    );

    std::cout << "Bot is running with v0.2 features enabled. Press Ctrl+C to stop.\n";
    client.run();
    return 0;
}
