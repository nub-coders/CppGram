// examples/hello.cpp — CppGram bot example.
// Demonstrates bot login, message handlers, filters, inline keyboards,
// and callback queries.
//
// Usage: Set CPPGRAM_API_ID, CPPGRAM_API_HASH, CPPGRAM_BOT_TOKEN env vars.
//        ./hello

#include "cppgram/client.hpp"
#include "cppgram/filters.hpp"
#include "cppgram/log.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    using namespace cppgram;
    Logger::instance().set_level(LogLevel::Info);

    const char* id_str = std::getenv("CPPGRAM_API_ID");
    const char* hash   = std::getenv("CPPGRAM_API_HASH");
    const char* token  = std::getenv("CPPGRAM_BOT_TOKEN");

    if (!id_str || !hash || !token) {
        std::cerr << "Set CPPGRAM_API_ID, CPPGRAM_API_HASH, CPPGRAM_BOT_TOKEN\n";
        return 1;
    }

    Client client(std::atoi(id_str), hash);
    client.loginBot(token);

    auto me = client.getMe();
    std::cout << "Logged in as: " << me.full_name()
              << " (@" << me.username << ")\n";

    // Echo handler — reply with the same text (private chats only).
    client.onMessage(
        Filters::text() && Filters::privateChat() && !Filters::bot(),
        [](Message msg) {
            std::cout << msg.sender.first_name << ": " << msg.text << "\n";
            msg.reply(msg.text);
        }
    );

    // /ping command — reply with an inline keyboard.
    client.onMessage(
        Filters::command("ping"),
        [](Message msg) {
            InlineKeyboard kb;
            kb.addButton(0, InlineKeyboardButton::callback("Pong!", "pong"));
            kb.addButton(0, InlineKeyboardButton::link("Docs", "https://github.com"));
            msg.reply("Choose an option:", kb);
        }
    );

    // /start command.
    client.onMessage(
        Filters::command("start"),
        [](Message msg) {
            msg.reply("Welcome to CppGram! Try /ping or /poll");
        }
    );

    // /poll command — create a poll.
    client.onMessage(
        Filters::command("poll"),
        [&client](Message msg) {
            client.sendPoll(
                msg.chat_id,
                "What's your favorite language?",
                {"C++", "Rust", "Python", "Go"}
            );
        }
    );

    // /dice command.
    client.onMessage(
        Filters::command("dice"),
        [&client](Message msg) {
            client.sendDice(msg.chat_id);
        }
    );

    // Handle callback queries from inline keyboards.
    client.onCallbackQuery([](CallbackQuery q) {
        std::cout << "Callback: " << q.data << " from user " << q.sender.id << "\n";
        if (q.data == "pong")
            q.answer("Pong received!", false);
        else
            q.answer();
    });

    // Log edited messages.
    client.onEditedMessage([](Message msg) {
        std::cout << "[edited] " << msg.text << "\n";
    });

    // Log deleted messages.
    client.onDeletedMessages([](ChatId chat, std::vector<MessageId> ids) {
        std::cout << "[deleted] " << ids.size()
                  << " message(s) in chat " << chat << "\n";
    });

    // Photo handler — acknowledge received photos.
    client.onMessage(
        Filters::photo(),
        [](Message msg) {
            if (msg.photo)
                msg.reply("Got your photo (" +
                          std::to_string(msg.photo->largest().width) + "x" +
                          std::to_string(msg.photo->largest().height) + ")");
        }
    );

    std::cout << "Bot is running. Press Ctrl+C to stop.\n";
    client.run();
    return 0;
}
