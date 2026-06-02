// examples/test_bot.cpp — Comprehensive CppGram test bot.
// Exercises most Client and Message methods via Telegram commands.
//
// Env vars: CPPGRAM_API_ID, CPPGRAM_API_HASH, CPPGRAM_BOT_TOKEN
// Commands:
//   /start        — welcome + command list
//   /me           — getMe() info
//   /echo <text>  — echo back with reply
//   /edit         — send, then edit the message
//   /delete       — send, then delete the message
//   /forward      — forward your command message back to you
//   /pin          — pin a message, then unpin after 3s
//   /react        — react to your message with thumbs up
//   /keyboard     — inline keyboard with callbacks
//   /replykb      — reply keyboard
//   /removekb     — remove reply keyboard
//   /poll         — regular poll
//   /quiz         — quiz poll
//   /dice         — roll a dice
//   /basketball   — basketball dice
//   /dart         — dart dice
//   /contact      — send a sample contact
//   /location     — send a sample location
//   /venue        — send a sample venue
//   /photo        — send a local test photo (if exists)
//   /document     — send a local test document (if exists)
//   /video        — send a local test video (if exists)
//   /audio        — send a local test audio (if exists)
//   /voicenote    — send a local test voice note (if exists)
//   /videonote    — send a local test video note (if exists)
//   /animation    — send a local test animation (if exists)
//   /sticker      — send a local test sticker (if exists)
//   /search <q>   — search messages in current chat
//   /globalsearch <q> — search messages across chats
//   /chatinfo     — get chat details
//   /memberme     — get the sender's current member status
//   /members      — get member count
//   /admins       — list administrators
//   /invite       — get invite link
//   /join         — join the current chat
//   /leave        — leave the current chat
//   /unpinall     — unpin all messages in the current chat
//   /settitle     — update the current chat title
//   /setdesc      — update the current chat description
//   /setphoto     — set a local test chat photo (if exists)
//   /delphoto     — delete the current chat photo
//   /history      — fetch last 5 messages
//   /stop         — graceful shutdown

#include "cppgram/client.hpp"
#include "cppgram/filters.hpp"
#include "cppgram/log.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

static bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

static std::string command_arg(const cppgram::Message& msg) {
    auto space = msg.text.find(' ');
    if (space == std::string::npos) return {};
    return msg.text.substr(space + 1);
}

static std::string chat_member_status_name(cppgram::ChatMemberStatus status) {
    using cppgram::ChatMemberStatus;
    switch (status) {
        case ChatMemberStatus::Creator:       return "creator";
        case ChatMemberStatus::Administrator: return "administrator";
        case ChatMemberStatus::Member:        return "member";
        case ChatMemberStatus::Restricted:    return "restricted";
        case ChatMemberStatus::Left:          return "left";
        case ChatMemberStatus::Banned:        return "banned";
    }
    return "unknown";
}

int main() {
    using namespace cppgram;
    using namespace std::chrono_literals;

    Logger::instance().set_level(LogLevel::Info);

    std::cout << "Starting CppGram test bot...\n";

    const char* id_env = std::getenv("CPPGRAM_API_ID");
    const char* hash_env = std::getenv("CPPGRAM_API_HASH");
    const char* token = std::getenv("CPPGRAM_BOT_TOKEN");
    if (!token) {
        std::cerr << "Set CPPGRAM_BOT_TOKEN environment variable\n";
        return 1;
    }

    std::int32_t api_id = id_env ? std::atoi(id_env) : 2040;
    std::string api_hash = hash_env ? hash_env : "b18441a1ff607e10a989891a5462e627";

    Client client(api_id, api_hash);

    auto send_local_media = [](Message msg,
                               const std::string& path,
                               const std::string& missing_text,
                               auto&& send_fn) {
        if (!file_exists(path)) {
            msg.reply(missing_text);
            return;
        }
        send_fn(InputFile::local(path));
    };

    auto download_attachment = [&client](ChatId chat_id, FileId file_id,
                                         const std::string& label) {
        auto info = client.getFile(file_id);
        auto path = client.downloadFile(info.file_id, "downloads");
        client.sendMessage(chat_id, label + " downloaded to: " + path);
    };

    auto register_handlers = [&]() {
    // ---- /start ----
    client.onMessage(
        Filters::command("start"),
        [](Message msg) {
            msg.reply(
                "CppGram Test Bot\n\n"
                "Commands:\n"
                "/me — bot info\n"
                "/echo <text> — echo back\n"
                "/edit — send + edit demo\n"
                "/delete — send + delete demo\n"
                "/forward — forward your message\n"
                "/pin — pin + unpin demo\n"
                "/react — reaction demo\n"
                "/keyboard — inline keyboard\n"
                "/replykb — reply keyboard\n"
                "/removekb — remove keyboard\n"
                "/poll — regular poll\n"
                "/quiz — quiz poll\n"
                "/dice — roll dice\n"
                "/basketball — basketball\n"
                "/dart — throw dart\n"
                "/contact — send contact\n"
                "/location — send location\n"
                "/venue — send venue\n"
                "/photo — send photo\n"
                "/document — send document\n"
                "/video — send video\n"
                "/audio — send audio\n"
                "/voicenote — send voice note\n"
                "/videonote — send video note\n"
                "/animation — send animation\n"
                "/sticker — send sticker\n"
                "/search <q> — search chat\n"
                "/globalsearch <q> — search all chats\n"
                "/chatinfo — chat details\n"
                "/memberme — sender member status\n"
                "/members — member count\n"
                "/admins — list admins\n"
                "/invite — invite link\n"
                "/join — join current chat\n"
                "/leave — leave current chat\n"
                "/unpinall — unpin all messages\n"
                "/settitle <title> — rename current chat\n"
                "/setdesc <text> — change description\n"
                "/setphoto — set a local photo\n"
                "/delphoto — delete current chat photo\n"
                "/history — last 5 messages\n"
                "/stop — shutdown bot"
            );
        }
    );

    // ---- /me ----
    client.onMessage(
        Filters::command("me"),
        [&client](Message msg) {
            auto me = client.getMe();
            std::ostringstream ss;
            ss << "Bot info:\n"
               << "  ID: " << me.id << "\n"
               << "  Name: " << me.full_name() << "\n"
               << "  Username: @" << me.username << "\n"
               << "  Is bot: " << (me.is_bot ? "yes" : "no");
            msg.reply(ss.str());
        }
    );

    // ---- /echo ----
    client.onMessage(
        Filters::command("echo"),
        [](Message msg) {
            auto body = command_arg(msg);
            if (body.empty()) {
                msg.reply("Usage: /echo <text>");
                return;
            }
            msg.reply(body);
        }
    );

    // ---- /edit ----
    client.onMessage(
        Filters::command("edit"),
        [&client](Message msg) {
            auto sent = msg.reply("This message will be edited...");
            std::this_thread::sleep_for(2s);
            client.editMessage(msg.chat_id, sent.id, "Message has been edited!");
        }
    );

    // ---- /delete ----
    client.onMessage(
        Filters::command("delete"),
        [&client](Message msg) {
            auto sent = msg.reply("This message will self-destruct in 3 seconds...");
            std::this_thread::sleep_for(3s);
            client.deleteMessages(msg.chat_id, {sent.id});
        }
    );

    // ---- /forward ----
    client.onMessage(
        Filters::command("forward"),
        [&client](Message msg) {
            client.forwardMessage(msg.chat_id, msg.id, msg.chat_id);
        }
    );

    // ---- /pin ----
    client.onMessage(
        Filters::command("pin"),
        [&client](Message msg) {
            auto sent = msg.reply("This message will be pinned for 5 seconds.");
            client.pinMessage(msg.chat_id, sent.id, true);
            std::this_thread::sleep_for(5s);
            client.unpinMessage(msg.chat_id, sent.id);
            client.sendMessage(msg.chat_id, "Unpinned.");
        }
    );

    // ---- /react ----
    client.onMessage(
        Filters::command("react"),
        [&client](Message msg) {
            client.setReaction(msg.chat_id, msg.id, "\xF0\x9F\x91\x8D");
            msg.reply("Reacted with thumbs up!");
        }
    );

    // ---- /keyboard — inline keyboard with callbacks ----
    client.onMessage(
        Filters::command("keyboard"),
        [](Message msg) {
            InlineKeyboard kb;
            kb.addButton(0, InlineKeyboardButton::callback("Button A", "btn_a"));
            kb.addButton(0, InlineKeyboardButton::callback("Button B", "btn_b"));
            kb.addButton(1, InlineKeyboardButton::callback("Button C", "btn_c"));
            kb.addButton(1, InlineKeyboardButton::link("GitHub", "https://github.com"));
            kb.addButton(2, InlineKeyboardButton::callback("Cancel", "btn_cancel"));
            msg.reply("Choose an option:", kb);
        }
    );

    // ---- /replykb — reply keyboard ----
    client.onMessage(
        Filters::command("replykb"),
        [&client](Message msg) {
            ReplyKeyboard rk;
            rk.addRow({
                ReplyKeyboardButton{"Option 1"},
                ReplyKeyboardButton{"Option 2"}
            });
            rk.addRow({
                ReplyKeyboardButton{"Option 3"},
                ReplyKeyboardButton{"Cancel"}
            });
            rk.resize_keyboard = true;
            rk.one_time_keyboard = true;
            client.sendMessage(msg.chat_id, "Pick from the keyboard:", rk);
        }
    );

    // ---- /removekb — remove reply keyboard ----
    client.onMessage(
        Filters::command("removekb"),
        [&client](Message msg) {
            RemoveKeyboard rmkb;
            client.sendMessage(msg.chat_id, "Keyboard removed.", rmkb);
        }
    );

    // ---- /poll ----
    client.onMessage(
        Filters::command("poll"),
        [&client](Message msg) {
            client.sendPoll(
                msg.chat_id,
                "What's the best C++ standard?",
                {"C++11", "C++14", "C++17", "C++20", "C++23"}
            );
        }
    );

    // ---- /quiz ----
    client.onMessage(
        Filters::command("quiz"),
        [&client](Message msg) {
            PollConfig cfg;
            cfg.type = PollType::Quiz;
            cfg.correct_option_id = 3;
            cfg.explanation = "C++20 introduced concepts, ranges, and coroutines!";
            client.sendPoll(
                msg.chat_id,
                "Which C++ standard introduced concepts?",
                {"C++11", "C++14", "C++17", "C++20"},
                cfg
            );
        }
    );

    // ---- /dice ----
    client.onMessage(
        Filters::command("dice"),
        [&client](Message msg) {
            auto sent = client.sendDice(msg.chat_id);
            if (sent.dice)
                client.sendMessage(msg.chat_id,
                    "You rolled: " + std::to_string(sent.dice->value));
        }
    );

    // ---- /basketball ----
    client.onMessage(
        Filters::command("basketball"),
        [&client](Message msg) {
            client.sendDice(msg.chat_id, "\xF0\x9F\x8F\x80");
        }
    );

    // ---- /dart ----
    client.onMessage(
        Filters::command("dart"),
        [&client](Message msg) {
            client.sendDice(msg.chat_id, "\xF0\x9F\x8E\xAF");
        }
    );

    // ---- /contact ----
    client.onMessage(
        Filters::command("contact"),
        [&client](Message msg) {
            Contact c;
            c.phone_number = "+1234567890";
            c.first_name = "Test";
            c.last_name = "Contact";
            client.sendContact(msg.chat_id, c);
        }
    );

    // ---- /location ----
    client.onMessage(
        Filters::command("location"),
        [&client](Message msg) {
            Location loc;
            loc.latitude = 48.8566;
            loc.longitude = 2.3522;
            client.sendLocation(msg.chat_id, loc);
            client.sendMessage(msg.chat_id, "Sent: Paris, France");
        }
    );

    // ---- /venue ----
    client.onMessage(
        Filters::command("venue"),
        [&client](Message msg) {
            Venue v;
            v.location.latitude = 48.8584;
            v.location.longitude = 2.2945;
            v.title = "Eiffel Tower";
            v.address = "Champ de Mars, 5 Av. Anatole France, 75007 Paris";
            client.sendVenue(msg.chat_id, v);
        }
    );

    // ---- /photo ----
    client.onMessage(
        Filters::command("photo"),
        [&client](Message msg) {
            std::string path = "test_photo.jpg";
            if (!file_exists(path)) {
                msg.reply("No test_photo.jpg found in working directory. "
                          "Place an image file there and try again.");
                return;
            }
            client.sendPhoto(msg.chat_id, InputFile::local(path), "Test photo from CppGram");
        }
    );

    // ---- /document ----
    client.onMessage(
        Filters::command("document"),
        [&client](Message msg) {
            std::string path = "test_doc.txt";
            if (!file_exists(path)) {
                msg.reply("No test_doc.txt found in working directory. "
                          "Place a file there and try again.");
                return;
            }
            client.sendDocument(msg.chat_id, InputFile::local(path), "Test document");
        }
    );

    // ---- /video ----
    client.onMessage(
        Filters::command("video"),
        [&client, &send_local_media](Message msg) {
            send_local_media(
                msg,
                "test_video.mp4",
                "No test_video.mp4 found in working directory.",
                [&client, &msg](const InputFile& file) {
                    client.sendVideo(msg.chat_id, file, "Test video from CppGram");
                }
            );
        }
    );

    // ---- /audio ----
    client.onMessage(
        Filters::command("audio"),
        [&client, &send_local_media](Message msg) {
            send_local_media(
                msg,
                "test_audio.mp3",
                "No test_audio.mp3 found in working directory.",
                [&client, &msg](const InputFile& file) {
                    client.sendAudio(msg.chat_id, file, "Test audio from CppGram");
                }
            );
        }
    );

    // ---- /voicenote ----
    client.onMessage(
        Filters::command("voicenote"),
        [&client, &send_local_media](Message msg) {
            send_local_media(
                msg,
                "test_voice.ogg",
                "No test_voice.ogg found in working directory.",
                [&client, &msg](const InputFile& file) {
                    client.sendVoiceNote(msg.chat_id, file, "Test voice note from CppGram");
                }
            );
        }
    );

    // ---- /videonote ----
    client.onMessage(
        Filters::command("videonote"),
        [&client, &send_local_media](Message msg) {
            send_local_media(
                msg,
                "test_video_note.mp4",
                "No test_video_note.mp4 found in working directory.",
                [&client, &msg](const InputFile& file) {
                    client.sendVideoNote(msg.chat_id, file);
                }
            );
        }
    );

    // ---- /animation ----
    client.onMessage(
        Filters::command("animation"),
        [&client, &send_local_media](Message msg) {
            send_local_media(
                msg,
                "test_animation.gif",
                "No test_animation.gif found in working directory.",
                [&client, &msg](const InputFile& file) {
                    client.sendAnimation(msg.chat_id, file, "Test animation from CppGram");
                }
            );
        }
    );

    // ---- /sticker ----
    client.onMessage(
        Filters::command("sticker"),
        [&client, &send_local_media](Message msg) {
            send_local_media(
                msg,
                "test_sticker.webp",
                "No test_sticker.webp found in working directory.",
                [&client, &msg](const InputFile& file) {
                    client.sendSticker(msg.chat_id, file);
                }
            );
        }
    );

    // ---- /globalsearch ----
    client.onMessage(
        Filters::command("globalsearch"),
        [&client](Message msg) {
            auto query = command_arg(msg);
            if (query.empty()) {
                msg.reply("Usage: /globalsearch <query>");
                return;
            }
            auto results = client.searchMessages(query, 10);
            std::ostringstream ss;
            ss << "Found " << results.size() << " result(s) for \"" << query
               << "\" across chats:\n";
            for (size_t i = 0; i < results.size() && i < 10; ++i) {
                ss << "\n[chat " << results[i].chat_id << ", msg " << results[i].id << "] ";
                if (!results[i].text.empty())
                    ss << results[i].text.substr(0, 80);
                else
                    ss << "(media)";
            }
            msg.reply(ss.str());
        }
    );

    // ---- /search ----
    client.onMessage(
        Filters::command("search"),
        [&client](Message msg) {
            auto query = command_arg(msg);
            if (query.empty()) {
                msg.reply("Usage: /search <query>");
                return;
            }
            auto results = client.searchChatMessages(msg.chat_id, query, 10);
            std::ostringstream ss;
            ss << "Found " << results.size() << " result(s) for \"" << query << "\":\n";
            for (size_t i = 0; i < results.size() && i < 10; ++i) {
                ss << "\n[" << results[i].id << "] ";
                if (!results[i].text.empty())
                    ss << results[i].text.substr(0, 80);
                else
                    ss << "(media)";
            }
            msg.reply(ss.str());
        }
    );

    // ---- /chatinfo ----
    client.onMessage(
        Filters::command("chatinfo"),
        [&client](Message msg) {
            auto chat = client.getChat(msg.chat_id);
            std::ostringstream ss;
            ss << "Chat info:\n"
               << "  ID: " << chat.id << "\n"
               << "  Title: " << chat.title << "\n"
               << "  Username: " << (chat.username.empty() ? "(none)" : "@" + chat.username) << "\n"
               << "  Type: ";
            switch (chat.type) {
                case ChatType::Private:    ss << "Private"; break;
                case ChatType::BasicGroup: ss << "Basic Group"; break;
                case ChatType::Supergroup: ss << "Supergroup"; break;
                case ChatType::Channel:    ss << "Channel"; break;
                case ChatType::Secret:     ss << "Secret"; break;
                default:                   ss << "Unknown"; break;
            }
            ss << "\n  Members: " << chat.member_count
               << "\n  Description: " << (chat.description.empty() ? "(none)" : chat.description)
               << "\n  Protected: " << (chat.has_protected_content ? "yes" : "no");
            msg.reply(ss.str());
        }
    );

    // ---- /memberme ----
    client.onMessage(
        Filters::command("memberme"),
        [&client](Message msg) {
            auto member = client.getChatMember(msg.chat_id, msg.sender.id);
            std::ostringstream ss;
            ss << "Your status in this chat: "
               << chat_member_status_name(member.status)
               << "\nName: " << member.user.full_name();
            if (!member.user.username.empty())
                ss << "\nUsername: @" << member.user.username;
            msg.reply(ss.str());
        }
    );

    // ---- /members ----
    client.onMessage(
        Filters::command("members"),
        [&client](Message msg) {
            int count = client.getChatMemberCount(msg.chat_id);
            msg.reply("Member count: " + std::to_string(count));
        }
    );

    // ---- /join ----
    client.onMessage(
        Filters::command("join"),
        [&client](Message msg) {
            client.joinChat(msg.chat_id);
            msg.reply("Requested join for this chat.");
        }
    );

    // ---- /leave ----
    client.onMessage(
        Filters::command("leave"),
        [&client](Message msg) {
            client.leaveChat(msg.chat_id);
            msg.reply("Requested leave for this chat.");
        }
    );

    // ---- /unpinall ----
    client.onMessage(
        Filters::command("unpinall"),
        [&client](Message msg) {
            client.unpinAllMessages(msg.chat_id);
            msg.reply("Unpinned all messages in this chat.");
        }
    );

    // ---- /settitle ----
    client.onMessage(
        Filters::command("settitle"),
        [&client](Message msg) {
            auto title = command_arg(msg);
            if (title.empty()) {
                msg.reply("Usage: /settitle <title>");
                return;
            }
            client.setChatTitle(msg.chat_id, title);
            msg.reply("Updated chat title.");
        }
    );

    // ---- /setdesc ----
    client.onMessage(
        Filters::command("setdesc"),
        [&client](Message msg) {
            auto desc = command_arg(msg);
            if (desc.empty()) {
                msg.reply("Usage: /setdesc <description>");
                return;
            }
            client.setChatDescription(msg.chat_id, desc);
            msg.reply("Updated chat description.");
        }
    );

    // ---- /setphoto ----
    client.onMessage(
        Filters::command("setphoto"),
        [&client](Message msg) {
            std::string path = "test_chat_photo.jpg";
            if (!file_exists(path)) {
                msg.reply("No test_chat_photo.jpg found in working directory.");
                return;
            }
            client.setChatPhoto(msg.chat_id, InputFile::local(path));
            msg.reply("Updated chat photo.");
        }
    );

    // ---- /delphoto ----
    client.onMessage(
        Filters::command("delphoto"),
        [&client](Message msg) {
            client.deleteChatPhoto(msg.chat_id);
            msg.reply("Deleted chat photo.");
        }
    );

    // ---- /admins ----
    client.onMessage(
        Filters::command("admins"),
        [&client](Message msg) {
            auto admins = client.getChatAdministrators(msg.chat_id);
            std::ostringstream ss;
            ss << "Administrators (" << admins.size() << "):\n";
            for (auto& a : admins) {
                ss << "  - " << a.user.full_name();
                if (!a.user.username.empty())
                    ss << " (@" << a.user.username << ")";
                if (a.status == ChatMemberStatus::Creator)
                    ss << " [creator]";
                if (!a.admin_rights.custom_title.empty())
                    ss << " (" << a.admin_rights.custom_title << ")";
                ss << "\n";
            }
            msg.reply(ss.str());
        }
    );

    // ---- /invite ----
    client.onMessage(
        Filters::command("invite"),
        [&client](Message msg) {
            auto link = client.getChatInviteLink(msg.chat_id);
            if (link.empty())
                msg.reply("Could not get invite link (might need admin rights).");
            else
                msg.reply("Invite link: " + link);
        }
    );

    // ---- /history ----
    client.onMessage(
        Filters::command("history"),
        [&client](Message msg) {
            auto history = client.getHistory(msg.chat_id, 5);
            std::ostringstream ss;
            ss << "Last " << history.size() << " message(s):\n";
            for (auto& m : history) {
                ss << "\n[" << m.id << "] " << m.sender.full_name() << ": ";
                if (!m.text.empty())
                    ss << m.text.substr(0, 100);
                else if (m.has_media())
                    ss << "(media)";
                else
                    ss << "(empty)";
            }
            msg.reply(ss.str());
        }
    );

    // ---- /stop — graceful shutdown ----
    client.onMessage(
        Filters::command("stop"),
        [&client](Message msg) {
            msg.reply("Shutting down...");
            client.stop();
        }
    );

    // ---- Callback query handler ----
    client.onCallbackQuery([&client](CallbackQuery q) {
        std::cout << "Callback from " << q.sender.full_name()
                  << ": " << q.data << "\n";

        if (q.data == "btn_cancel") {
            q.answer("Cancelled!", false);
            client.editMessage(q.chat_id, q.message_id, "Cancelled.");
        } else {
            q.answer("You pressed: " + q.data, false);
            client.editMessage(q.chat_id, q.message_id,
                "You selected: " + q.data);
        }
    });

    // ---- Edited message handler ----
    client.onEditedMessage([](Message msg) {
        std::cout << "[edited] chat=" << msg.chat_id
                  << " msg=" << msg.id
                  << " new_text=\"" << msg.text << "\"\n";
    });

    // ---- Deleted messages handler ----
    client.onDeletedMessages([](ChatId chat, std::vector<MessageId> ids) {
        std::cout << "[deleted] " << ids.size()
                  << " message(s) in chat " << chat << "\n";
    });

    // ---- Photo handler ----
    client.onMessage(
        Filters::photo(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received photo";
            if (msg.photo) {
                auto& lg = msg.photo->largest();
                ss << " (" << lg.width << "x" << lg.height
                   << ", " << lg.file_size << " bytes, "
                   << msg.photo->sizes.size() << " size(s))";
                download_attachment(msg.chat_id, lg.file_id, "Photo");
            }
            if (!msg.caption.empty())
                ss << "\nCaption: " << msg.caption;
            msg.reply(ss.str());
        }
    );

    // ---- Video handler ----
    client.onMessage(
        Filters::video(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received video";
            if (msg.video) {
                ss << " (" << msg.video->width << "x" << msg.video->height
                   << ", " << msg.video->duration << "s)";
                download_attachment(msg.chat_id, msg.video->file_id, "Video");
            }
            msg.reply(ss.str());
        }
    );

    // ---- Document handler ----
    client.onMessage(
        Filters::document(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received document";
            if (msg.document) {
                ss << ": " << msg.document->file_name
                   << " (" << msg.document->mime_type << ", "
                   << msg.document->file_size << " bytes)";
                download_attachment(msg.chat_id, msg.document->file_id, "Document");
            }
            msg.reply(ss.str());
        }
    );

    // ---- Sticker handler ----
    client.onMessage(
        Filters::sticker(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received sticker";
            if (msg.sticker) {
                ss << " (emoji: " << msg.sticker->emoji
                   << ", set: " << msg.sticker->set_name << ")";
                download_attachment(msg.chat_id, msg.sticker->file_id, "Sticker");
            }
            msg.reply(ss.str());
        }
    );

    // ---- Voice handler ----
    client.onMessage(
        Filters::voice(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received voice note";
            if (msg.voice_note) {
                ss << " (" << msg.voice_note->duration << "s)";
                download_attachment(msg.chat_id, msg.voice_note->file_id, "Voice note");
            }
            msg.reply(ss.str());
        }
    );

    // ---- Video note handler ----
    client.onMessage(
        Filters::videoNote(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received video note";
            if (msg.video_note) {
                ss << " (" << msg.video_note->duration << "s, length "
                   << msg.video_note->length << ")";
                download_attachment(msg.chat_id, msg.video_note->file_id, "Video note");
            }
            msg.reply(ss.str());
        }
    );

    // ---- Audio handler ----
    client.onMessage(
        Filters::audio(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received audio";
            if (msg.audio) {
                ss << " (" << msg.audio->duration << "s";
                if (!msg.audio->title.empty())
                    ss << ", title: " << msg.audio->title;
                if (!msg.audio->performer.empty())
                    ss << ", performer: " << msg.audio->performer;
                ss << ")";
                download_attachment(msg.chat_id, msg.audio->file_id, "Audio");
            }
            msg.reply(ss.str());
        }
    );

    // ---- Animation/GIF handler ----
    client.onMessage(
        Filters::animation(),
        [&download_attachment](Message msg) {
            std::ostringstream ss;
            ss << "Received animation/GIF";
            if (msg.animation) {
                ss << " (" << msg.animation->width << "x" << msg.animation->height
                   << ", " << msg.animation->duration << "s)";
                download_attachment(msg.chat_id, msg.animation->file_id, "Animation");
            }
            msg.reply(ss.str());
        }
    );

    // ---- Poll received handler ----
    client.onMessage(
        Filters::pollMsg(),
        [](Message msg) {
            if (msg.poll) {
                std::ostringstream ss;
                ss << "Received poll: " << msg.poll->question << "\n";
                for (size_t i = 0; i < msg.poll->options.size(); ++i)
                    ss << "  " << i << ". " << msg.poll->options[i].text << "\n";
                msg.reply(ss.str());
            }
        }
    );

    // ---- Location handler ----
    client.onMessage(
        Filters::locationMsg(),
        [](Message msg) {
            if (msg.location) {
                std::ostringstream ss;
                ss << "Received location: "
                   << msg.location->latitude << ", " << msg.location->longitude;
                msg.reply(ss.str());
            }
        }
    );

    // ---- Contact handler ----
    client.onMessage(
        Filters::contactMsg(),
        [](Message msg) {
            if (msg.contact) {
                std::ostringstream ss;
                ss << "Received contact: "
                   << msg.contact->first_name << " " << msg.contact->last_name
                   << " (" << msg.contact->phone_number << ")";
                msg.reply(ss.str());
            }
        }
    );
    };

    register_handlers();
    std::cout << "Handlers registered.\n" << std::flush;
    std::cout << "Attempting bot login...\n" << std::flush;
    client.loginBot(token);

    auto me = client.getMe();
    std::cout << "Logged in as: " << me.full_name()
              << " (@" << me.username << ") [id: " << me.id << "]\n" << std::flush;
    std::cout << "Bot is ready.\n" << std::flush;

    std::cout << "Test bot is running. Send /start for commands. Ctrl+C to stop.\n" << std::flush;
    client.run();

    std::cout << "Bot stopped.\n";
    return 0;
}
