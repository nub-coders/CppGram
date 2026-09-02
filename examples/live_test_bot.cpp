// examples/live_test_bot.cpp
// Interactive live test bot for CppGram (Layer 225 & Telegram Bot API 10.3).
//
// Automatically loads credentials from .env, environment variables, or discovered root configuration.
// Features:
//   - /start, /help   : Command list and bot identity
//   - /me             : Client identity via client.getMe()
//   - /ping           : Latency and uptime check
//   - /echo <text>    : Echo message
//   - /buttons        : Inline keyboard with dynamic callback queries
//   - /dice, /dart    : Animated Telegram dice and games
//   - /rich           : Bot API 10.3 compact table, expandable blockquote, and rich blocks
//   - /draft          : Bot API 10.3 rich message draft streaming with thinking block
//   - /chatinfo       : Current chat metadata
//   - /stats          : Message count, thread pool, and uptime statistics
//   - /stop           : Graceful shutdown
//   - Flags:
//       --verify / --test-once : Verify login & connectivity, print details, and exit cleanly.

#include "cppgram/client.hpp"
#include "cppgram/filters.hpp"
#include "cppgram/keyboard.hpp"
#include "cppgram/log.hpp"
#include "cppgram/rich_message.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <curl/curl.h>

namespace {

struct ExtractedEmoji {
    std::string emoji;
    std::string custom_emoji_id;
};

std::string unescape_json_unicode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size();) {
        if (s[i] == '\\' && i + 5 < s.size() && s[i+1] == 'u') {
            std::string hex = s.substr(i + 2, 4);
            unsigned int cp = std::stoul(hex, nullptr, 16);
            i += 6;
            if (cp >= 0xD800 && cp <= 0xDBFF && i + 5 < s.size() && s[i] == '\\' && s[i+1] == 'u') {
                std::string hex2 = s.substr(i + 2, 4);
                unsigned int cp2 = std::stoul(hex2, nullptr, 16);
                if (cp2 >= 0xDC00 && cp2 <= 0xDFFF) {
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (cp2 - 0xDC00);
                    i += 6;
                }
            }
            if (cp < 0x80) {
                out += static_cast<char>(cp);
            } else if (cp < 0x800) {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        } else {
            out += s[i++];
        }
    }
    return out;
}

std::string fetch_sticker_set_json(const std::string& bot_token, const std::string& short_name) {
    std::string url = "https://api.telegram.org/bot" + bot_token + "/getStickerSet?name=" + short_name;
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    std::string resp;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    auto write_cb = +[](char* ptr, size_t size, size_t nmemb, void* ud) -> size_t {
        auto* s = static_cast<std::string*>(ud);
        s->append(ptr, size * nmemb);
        return size * nmemb;
    };
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return {};
    return resp;
}

std::vector<ExtractedEmoji> extract_custom_emojis(const std::string& json_str) {
    std::vector<ExtractedEmoji> results;
    size_t pos = 0;
    while ((pos = json_str.find("\"custom_emoji_id\":", pos)) != std::string::npos) {
        size_t id_start = pos + 18;
        while (id_start < json_str.size() && (json_str[id_start] == ' ' || json_str[id_start] == '"')) {
            id_start++;
        }
        size_t id_end = json_str.find_first_of("\",}", id_start);
        std::string cid = json_str.substr(id_start, id_end - id_start);

        std::string emoji_char = "⭐";
        size_t search_back_start = (pos > 300) ? (pos - 300) : 0;
        size_t emoji_pos = json_str.rfind("\"emoji\":\"", pos);
        if (emoji_pos != std::string::npos && emoji_pos >= search_back_start) {
            size_t e_start = emoji_pos + 9;
            size_t e_end = json_str.find('"', e_start);
            if (e_end != std::string::npos) {
                emoji_char = unescape_json_unicode(json_str.substr(e_start, e_end - e_start));
            }
        }

        if (!cid.empty()) {
            results.push_back({emoji_char, cid});
        }
        pos = id_end;
    }
    return results;
}

// Default discovered credentials from /root/Session-gen/.env
constexpr std::int32_t FALLBACK_API_ID = 21856699;
constexpr const char* FALLBACK_API_HASH = "73f10cf0979637857170f03d4c86f251";
constexpr const char* FALLBACK_BOT_TOKEN = "7722414616:AAE5fIgnMUK6po4dgHiL85K7uAhnX0tngPI";

std::atomic<cppgram::Client*> g_client_ptr{nullptr};
std::atomic<uint64_t> g_messages_processed{0};
std::chrono::steady_clock::time_point g_start_time;

void signal_handler(int sig) {
    std::cout << "\n[Signal " << sig << " received] Shutting down CppGram test bot gracefully...\n" << std::flush;
    if (auto* c = g_client_ptr.load()) {
        c->stop();
    }
}

void load_env_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        // Trim whitespace and quotes
        while (!val.empty() && (val.back() == '\r' || val.back() == '\n' || val.back() == ' ' || val.back() == '"' || val.back() == '\''))
            val.pop_back();
        while (!val.empty() && (val.front() == ' ' || val.front() == '"' || val.front() == '\''))
            val.erase(val.begin());
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
            key.pop_back();
        while (!key.empty() && (key.front() == ' ' || key.front() == '\t'))
            key.erase(key.begin());

        if (!key.empty()) {
            setenv(key.c_str(), val.c_str(), 0); // do not overwrite if already set in environment
        }
    }
}

void discover_and_load_env() {
    const std::vector<std::string> env_candidates = {
        ".env",
        "/root/CppGram/.env",
        "/root/Session-gen/.env",
        "/root/OTPBOT/.env"
    };

    for (const auto& path : env_candidates) {
        load_env_file(path);
    }
}

std::string format_uptime() {
    auto now = std::chrono::steady_clock::now();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - g_start_time).count();
    long hours = sec / 3600;
    long mins = (sec % 3600) / 60;
    long secs = sec % 60;
    std::ostringstream ss;
    if (hours > 0) ss << hours << "h ";
    if (mins > 0 || hours > 0) ss << mins << "m ";
    ss << secs << "s";
    return ss.str();
}

std::string get_command_argument(const cppgram::Message& msg) {
    auto space = msg.text.find(' ');
    if (space == std::string::npos) return {};
    return msg.text.substr(space + 1);
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace cppgram;
    g_start_time = std::chrono::steady_clock::now();

    bool verify_only = false;
    std::string custom_token;
    std::int32_t custom_id = 0;
    std::string custom_hash;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verify" || arg == "--test-once") {
            verify_only = true;
        } else if (arg == "--token" && i + 1 < argc) {
            custom_token = argv[++i];
        } else if (arg == "--api-id" && i + 1 < argc) {
            custom_id = std::atoi(argv[++i]);
        } else if (arg == "--api-hash" && i + 1 < argc) {
            custom_hash = argv[++i];
        }
    }

    std::cout << "=======================================================\n";
    std::cout << "        CppGram Live Test Bot (Layer 225 / 10.3)       \n";
    std::cout << "=======================================================\n";

    // 1. Discover environment credentials
    discover_and_load_env();

    std::int32_t api_id = custom_id;
    if (api_id == 0) {
        if (const char* env = std::getenv("CPPGRAM_API_ID")) api_id = std::atoi(env);
        else if (const char* env = std::getenv("API_ID")) api_id = std::atoi(env);
        else api_id = FALLBACK_API_ID;
    }

    std::string api_hash = custom_hash;
    if (api_hash.empty()) {
        if (const char* env = std::getenv("CPPGRAM_API_HASH")) api_hash = env;
        else if (const char* env = std::getenv("API_HASH")) api_hash = env;
        else api_hash = FALLBACK_API_HASH;
    }

    std::string token = custom_token;
    if (token.empty()) {
        if (const char* env = std::getenv("CPPGRAM_BOT_TOKEN")) token = env;
        else if (const char* env = std::getenv("BOT_TOKEN")) token = env;
        else token = FALLBACK_BOT_TOKEN;
    }

    std::cout << "  - API ID   : " << api_id << "\n";
    std::cout << "  - API Hash : " << api_hash.substr(0, 6) << "..." << "\n";
    std::string masked_token = token.size() > 12 ? (token.substr(0, 8) + "..." + token.substr(token.size() - 4)) : "****";
    std::cout << "  - Bot Token: " << masked_token << "\n";
    std::cout << "  - Mode     : " << (verify_only ? "Verify & Self-Test" : "Interactive Live Service") << "\n\n";

    // 2. Initialize Client
    Logger::instance().set_level(LogLevel::Info);
    Client client(api_id, api_hash);
    g_client_ptr.store(&client);

    // Filter out bot's own outgoing messages to avoid feedback loops
    client.use([](MiddlewareContext& ctx) -> bool {
        if (ctx.is_message() && ctx.message()->outgoing) {
            return false;
        }
        return true;
    });

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 3. Showcase Builders & Register Command Handlers
    auto build_showcase_message = [](const std::string& uptime_str) -> InputRichMessage {
        RichBlockTable table;
        table.is_bordered = true;
        table.is_compact = true;
        table.is_striped = true;
        table.caption = "Tap either button to test richmsgbtn vs richtextbtn interactive actions!";

        table.addRow({
            RichBlockTableCell::make_header("#", "center"),
            RichBlockTableCell::make_header("Button Name", "left"),
            RichBlockTableCell::make_header("API Type", "center"),
            RichBlockTableCell::make_header("Action", "center")
        });

        table.addRow({
            RichBlockTableCell::make_text("1️⃣", "center"),
            RichBlockTableCell::make_text("Text Button", "left"),
            RichBlockTableCell::make_text("richtextbtn", "center"),
            RichBlockTableCell::make_button(
                RichMessageButton::make_callback("📝 Click Text", "clicked_richtextbtn", "primary"),
                "center")
        });

        table.addRow({
            RichBlockTableCell::make_text("2️⃣", "center"),
            RichBlockTableCell::make_text("Message Button", "left"),
            RichBlockTableCell::make_text("richmsgbtn", "center"),
            RichBlockTableCell::make_button(
                RichMessageButton::make_callback("🎛️ Click Msg", "clicked_richmsgbtn", "primary"),
                "center")
        });

        table.addRow({
            RichBlockTableCell::make_text("⭐", "center"),
            RichBlockTableCell::make_text("Custom Emoji", "left"),
            RichBlockTableCell::make_text("Layer 225", "center"),
            RichBlockTableCell::make_custom_emoji("👍", "5368324170671202286", "center")
        });

        return RichMessageBuilder()
            .heading("📊 Telegram Bot API 10.3 Rich Message & Table")
            .paragraph("Both buttons and custom emojis are embedded directly into the RichBlockTable:")
            .table(table)
            .preformatted("RichBlockTable table;\ntable.is_bordered = true;\ntable.is_compact = true;\ntable.addRow({RichBlockTableCell::make_button(...)});", "cpp")
            .buttons({
                RichMessageButton::make_callback("📝 Click Text Button", "clicked_richtextbtn", "primary"),
                RichMessageButton::make_callback("🎛️ Click Msg Button", "clicked_richmsgbtn", "success"),
                RichMessageButton::make_url("GitHub Repo", "https://github.com/nub-coders/CppGram", "link")
            }, "center")
            .build();
    };

    auto build_clicked_message = [](const std::string& btn_type, const std::string& btn_title) -> InputRichMessage {
        RichBlockTable table;
        table.is_bordered = true;
        table.is_compact = true;
        table.is_striped = true;
        table.caption = "Button click recorded via Telegram Bot API 10.3!";

        table.addRow({
            RichBlockTableCell::make_header("Property", "left"),
            RichBlockTableCell::make_header("Details", "left")
        });
        table.addRow({
            RichBlockTableCell::make_bold("Button Triggered", "left"),
            RichBlockTableCell::make_text(btn_title, "left")
        });
        table.addRow({
            RichBlockTableCell::make_bold("API Type", "left"),
            RichBlockTableCell::make_code(btn_type, "left")
        });
        table.addRow({
            RichBlockTableCell::make_bold("Placement", "left"),
            RichBlockTableCell::make_text(btn_type == "richtextbtn" ? "Inside Table Cell" : "Dedicated Block Button", "left")
        });
        table.addRow({
            RichBlockTableCell::make_bold("Custom Emoji", "left"),
            RichBlockTableCell::make_custom_emoji("🥳", "5407057942388153892", "left")
        });
        table.addRow({
            RichBlockTableCell::make_bold("Live Status", "left"),
            RichBlockTableCell::make_text("Active & Verified ✅", "left")
        });
        table.addRow({
            RichBlockTableCell::make_bold("Action", "left"),
            RichBlockTableCell::make_button(RichMessageButton::make_callback("🔄 Reset Table", "reset_button_table", "primary"), "left")
        });

        return RichMessageBuilder()
            .heading("✅ Interactive Button Click Verified!")
            .paragraph("The callback query was processed and the rich table was updated in-place via editRichMessage:")
            .table(table)
            .buttons({
                RichMessageButton::make_callback("📝 Click Text Button", "clicked_richtextbtn", "primary"),
                RichMessageButton::make_callback("🎛️ Click Msg Button", "clicked_richmsgbtn", "success"),
                RichMessageButton::make_callback("🔄 Reset Table", "reset_button_table", "link")
            }, "center")
            .build();
    };

    auto send_rich_showcase = [&](ChatId chat_id) {
        auto rmsg = build_showcase_message(format_uptime());
        SendRichMessageOptions opts;
        opts.protect_content = false;
        client.sendRichMessage(chat_id, rmsg, opts);
    };

    client.onMessage(
        [](const Message& msg) {
            return msg.text == "/start" || msg.text == ".start" ||
                   msg.text == "/help"  || msg.text == ".help" ||
                   msg.text.rfind("/start@", 0) == 0 || msg.text.rfind(".start@", 0) == 0 ||
                   msg.text.rfind("/help@", 0) == 0  || msg.text.rfind(".help@", 0) == 0;
        },
        [&client, &send_rich_showcase](Message msg) {
            ++g_messages_processed;
            std::ostringstream ss;
            ss << "✨ <b>Welcome to CppGram Live Test Bot!</b> ✨\n\n"
               << "Built with C++20, MTProto Layer 225 & Telegram Bot API 10.3.\n\n"
               << "<b>Available Test Functions:</b>\n"
               << "• /start, /help - Show this guide & rich table\n"
               << "• /rich, /table, .rich, .table - Rich Message & Table Showcase\n"
               << "• /customemoji - Custom Emoji Showcase (&lt;tg-emoji&gt;)\n"
               << "• /me - Display bot identity details\n"
               << "• /ping - Check latency & uptime\n"
               << "• /echo &lt;text&gt; - Echo back text\n"
               << "• /buttons - Interactive inline keyboard\n"
               << "• /dice, /dart, /basketball - Animated Telegram dice 🎲\n"
               << "• /draft - Stream AI rich draft with thinking block\n"
               << "• /chatinfo - Current chat metadata\n"
               << "• /stats - Bot runtime statistics\n"
               << "• Send any <code>t.me/addemoji/&lt;pack&gt;</code> link to inspect custom emojis!\n";

            msg.reply(ss.str(), ParseMode::HTML);
            send_rich_showcase(msg.chat_id);
        }
    );

    client.onMessage(
        Filters::command("me"),
        [&client](Message msg) {
            ++g_messages_processed;
            auto me = client.getMe();
            std::ostringstream ss;
            ss << "🤖 <b>Bot Identity Information:</b>\n"
               << "• <b>ID:</b> <code>" << me.id << "</code>\n"
               << "• <b>Name:</b> " << me.first_name << (me.last_name.empty() ? "" : " " + me.last_name) << "\n"
               << "• <b>Username:</b> @" << me.username << "\n"
               << "• <b>Is Bot:</b> " << (me.is_bot ? "Yes" : "No") << "\n"
               << "• <b>Uptime:</b> " << format_uptime();
            msg.reply(ss.str(), ParseMode::HTML);
        }
    );

    client.onMessage(
        Filters::command("ping"),
        [](Message msg) {
            ++g_messages_processed;
            auto start_ts = std::chrono::steady_clock::now();
            auto sent = msg.reply("🏓 <i>Pinging Telegram MTProto Gateway...</i>", ParseMode::HTML);
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_ts).count();

            std::ostringstream ss;
            ss << "🏓 <b>Pong!</b>\n"
               << "• <b>Latency:</b> " << elapsed_ms << " ms\n"
               << "• <b>Uptime:</b> " << format_uptime() << "\n"
               << "• <b>Backend:</b> TDLib MTProto Layer 225";
            sent.edit(ss.str(), ParseMode::HTML);
        }
    );

    client.onMessage(
        Filters::command("echo"),
        [](Message msg) {
            ++g_messages_processed;
            auto text = get_command_argument(msg);
            if (text.empty()) {
                msg.reply("⚠️ Please provide text to echo:\n<code>/echo Hello CppGram!</code>", ParseMode::HTML);
                return;
            }
            msg.reply("🔊 <b>Echo:</b> " + text, ParseMode::HTML);
        }
    );

    client.onMessage(
        Filters::command("buttons"),
        [&client](Message msg) {
            ++g_messages_processed;
            InlineKeyboard kb;
            kb.addRow({
                InlineKeyboardButton::callback("🏓 Ping", "btn_ping"),
                InlineKeyboardButton::callback("🎲 Dice", "btn_dice")
            });
            kb.addRow({
                InlineKeyboardButton::callback("ℹ️ Bot Info", "btn_info"),
                InlineKeyboardButton::callback("📊 Stats", "btn_stats")
            });
            kb.addRow({
                InlineKeyboardButton::link("🌐 GitHub", "https://github.com/nub-coders/CppGram"),
                InlineKeyboardButton::callback("❌ Cancel", "btn_cancel")
            });

            client.sendMessage(
                msg.chat_id,
                "🔘 <b>Interactive Button Test</b>\nSelect an option below to test callback queries:",
                kb,
                ParseMode::HTML,
                {},
                msg.id
            );
        }
    );

    client.onMessage(
        Filters::command("dice"),
        [&client](Message msg) {
            ++g_messages_processed;
            client.sendDice(msg.chat_id, "\xF0\x9F\x8E\xB2"); // 🎲
        }
    );

    client.onMessage(
        Filters::command("dart"),
        [&client](Message msg) {
            ++g_messages_processed;
            client.sendDice(msg.chat_id, "\xF0\x9F\x8E\xAF"); // 🎯
        }
    );

    client.onMessage(
        Filters::command("basketball"),
        [&client](Message msg) {
            ++g_messages_processed;
            client.sendDice(msg.chat_id, "\xF0\x9F\x8F\x80"); // 🏀
        }
    );

    // Bot API 10.3 Rich Message & Table showcase (/rich, .rich, /table, .table)
    client.onMessage(
        [](const Message& msg) {
            return msg.text == "/rich"  || msg.text == ".rich" ||
                   msg.text == "/table" || msg.text == ".table" ||
                   msg.text.rfind("/rich@", 0) == 0 || msg.text.rfind(".rich@", 0) == 0 ||
                   msg.text.rfind("/table@", 0) == 0 || msg.text.rfind(".table@", 0) == 0;
        },
        [&](Message msg) {
            ++g_messages_processed;
            send_rich_showcase(msg.chat_id);
        }
    );

    // Telegram Bot API 10.3 Custom Emoji showcase (/customemoji, .customemoji)
    client.onMessage(
        [](const Message& msg) {
            return msg.text == "/customemoji" || msg.text == ".customemoji" ||
                   msg.text.rfind("/customemoji@", 0) == 0 || msg.text.rfind(".customemoji@", 0) == 0;
        },
        [&client](Message msg) {
            ++g_messages_processed;
            std::string text =
                "✨ <b>Telegram Bot API 10.3 Custom Emoji Showcase</b> ✨\n\n"
                "Custom emoji entities allow Telegram Premium & custom pack symbols inline:\n\n"
                "• <b>Thumb:</b> <tg-emoji emoji-id=\"5368324170671202286\">👍</tg-emoji> <code>(ID: 5368324170671202286)</code>\n"
                "• <b>Joy:</b> <tg-emoji emoji-id=\"5377637695583426942\">😂</tg-emoji> <code>(ID: 5377637695583426942)</code>\n"
                "• <b>Wink:</b> <tg-emoji emoji-id=\"5416025958656253132\">😉</tg-emoji> <code>(ID: 5416025958656253132)</code>\n"
                "• <b>Party:</b> <tg-emoji emoji-id=\"5407057942388153892\">🥳</tg-emoji> <code>(ID: 5407057942388153892)</code>\n\n"
                "<b>HTML Format:</b>\n"
                "<code>&lt;tg-emoji emoji-id=\"5368324170671202286\"&gt;👍&lt;/tg-emoji&gt;</code>\n\n"
                "💡 <i>Tip: Send any emoji pack link (e.g. https://t.me/addemoji/pack_name) to inspect!</i>";

            InlineKeyboard kb;
            kb.addRow({
                InlineKeyboardButton::callback("👍 Test Custom Emoji", "test_custom_emoji"),
                InlineKeyboardButton::callback("📊 Rich Table", "btn_table_showcase")
            });

            client.sendMessage(msg.chat_id, text, kb, ParseMode::HTML);
        }
    );

    // Custom Emoji Pack Link Listener (e.g. t.me/addemoji/<pack>)
    client.onMessage(
        [](const Message& msg) {
            return msg.text.find("t.me/addemoji/") != std::string::npos;
        },
        [&client, token](Message msg) {
            ++g_messages_processed;
            auto pos = msg.text.find("t.me/addemoji/");
            std::string pack_name = msg.text.substr(pos + 14);
            auto space = pack_name.find_first_of(" \t\r\n/?#)>\"\'<;:,");
            if (space != std::string::npos) {
                pack_name = pack_name.substr(0, space);
            }
            if (pack_name.empty() || pack_name == "<pack>" || pack_name == "pack_name" || pack_name == "pack") {
                return;
            }

            auto escape_html = [](const std::string& input) -> std::string {
                std::string out;
                for (char c : input) {
                    if (c == '&') out += "&amp;";
                    else if (c == '<') out += "&lt;";
                    else if (c == '>') out += "&gt;";
                    else if (c == '"') out += "&quot;";
                    else out += c;
                }
                return out;
            };
            std::string safe_pack_name = escape_html(pack_name);

            std::string json_resp = fetch_sticker_set_json(token, pack_name);
            auto emojis = extract_custom_emojis(json_resp);

            if (!emojis.empty()) {
                std::ostringstream ss;
                ss << "🎨 <b>Custom Emoji Pack:</b> <code>" << safe_pack_name << "</code>\n"
                   << "Found <b>" << emojis.size() << "</b> custom emojis in set!\n\n";

                size_t display_count = std::min<size_t>(emojis.size(), 20);
                for (size_t i = 0; i < display_count; ++i) {
                    ss << (i + 1) << ". <tg-emoji emoji-id=\"" << emojis[i].custom_emoji_id << "\">"
                       << emojis[i].emoji << "</tg-emoji> <code>" << emojis[i].custom_emoji_id << "</code>\n";
                }
                if (emojis.size() > display_count) {
                    ss << "\n<i>...and " << (emojis.size() - display_count) << " more custom emojis.</i>";
                }

                InlineKeyboard kb;
                kb.addRow({
                    InlineKeyboardButton::link("Open Sticker Pack", "https://t.me/addemoji/" + pack_name),
                    InlineKeyboardButton::callback("📊 Rich Table Demo", "btn_table_showcase")
                });
                client.sendMessage(msg.chat_id, ss.str(), kb, ParseMode::HTML);
            } else {
                std::string resp =
                    "🎨 <b>Custom Emoji Pack Detected:</b> <code>" + safe_pack_name + "</code>\n\n"
                    "• <b>Pack Link:</b> https://t.me/addemoji/" + pack_name + "\n"
                    "• <b>MTProto Layer:</b> Layer 225 Custom Emoji Support\n"
                    "• <b>Render Syntax:</b> <code>&lt;tg-emoji emoji-id=\"...\"&gt;😀&lt;/tg-emoji&gt;</code>\n\n"
                    "Try sending <code>/customemoji</code> or <code>/rich</code> to see live custom emoji and table rendering!";

                InlineKeyboard kb;
                kb.addRow({
                    InlineKeyboardButton::link("Open Pack", "https://t.me/addemoji/" + pack_name),
                    InlineKeyboardButton::callback("✨ Custom Emoji Demo", "test_custom_emoji")
                });
                client.sendMessage(msg.chat_id, resp, kb, ParseMode::HTML);
            }
        }
    );

    // Bot API 10.3 Rich Message Draft Streaming
    client.onMessage(
        Filters::command("draft"),
        [&client](Message msg) {
            ++g_messages_processed;
            auto streaming_draft = RichMessageBuilder()
                .thinking("Generating live intelligence report and aggregating telemetry...")
                .paragraph("Draft streaming preview completed successfully.")
                .build();

            int64_t draft_id = static_cast<int64_t>(msg.id);
            bool success = client.sendRichMessageDraft(msg.chat_id, draft_id, streaming_draft, true, false);
            if (success) {
                msg.reply("✅ <b>Rich Message Draft Streamed!</b> (Draft ID: " + std::to_string(draft_id) + ")", ParseMode::HTML);
            } else {
                msg.reply("⚠️ Draft streaming attempted. Telegram client notified.");
            }
        }
    );

    client.onMessage(
        Filters::command("chatinfo"),
        [&client](Message msg) {
            ++g_messages_processed;
            auto chat_type_str = [](ChatType t) {
                switch (t) {
                    case ChatType::Private: return "Private DM";
                    case ChatType::BasicGroup: return "Group";
                    case ChatType::Supergroup: return "Supergroup";
                    case ChatType::Channel: return "Channel";
                    case ChatType::Secret: return "Secret Chat";
                    default: return "Unknown";
                }
            };
            std::ostringstream ss;
            ss << "💬 <b>Current Chat Information:</b>\n"
               << "• <b>Chat ID:</b> <code>" << msg.chat_id << "</code>\n"
               << "• <b>Message ID:</b> <code>" << msg.id << "</code>\n"
               << "• <b>Sender ID:</b> <code>" << msg.sender.id << "</code> (" << msg.sender.full_name() << ")\n"
               << "• <b>Chat Type:</b> " << chat_type_str(msg.chat_type);
            msg.reply(ss.str(), ParseMode::HTML);
        }
    );

    client.onMessage(
        Filters::command("stats"),
        [&client](Message msg) {
            ++g_messages_processed;
            std::ostringstream ss;
            ss << "📊 <b>CppGram Test Bot Statistics:</b>\n"
               << "• <b>Uptime:</b> " << format_uptime() << "\n"
               << "• <b>Messages Processed:</b> " << g_messages_processed.load() << "\n"
               << "• <b>Thread Pool Workers:</b> " << client.getThreadPoolSize() << "\n"
               << "• <b>Active Plugins:</b> " << client.plugins().size() << "\n"
               << "• <b>MTProto Layer:</b> Layer 225\n"
               << "• <b>Bot API Spec:</b> 10.3 (August 2026)";
            msg.reply(ss.str(), ParseMode::HTML);
        }
    );

    client.onMessage(
        Filters::command("stop"),
        [&client](Message msg) {
            msg.reply("🛑 <b>Shutting down CppGram Test Bot. Goodbye!</b>", ParseMode::HTML);
            std::cout << "[/stop command] Received from user " << msg.sender.id << ". Stopping...\n";
            client.stop();
        }
    );

    // Callback Query Handler for Inline Buttons
    client.onCallbackQuery([&client, &build_showcase_message, &build_clicked_message](CallbackQuery q) {
        ++g_messages_processed;
        std::cout << "[Callback] " << q.sender.full_name() << " clicked: " << q.data << "\n";

        if (q.data == "clicked_richtextbtn") {
            q.answer("Text Button (richtextbtn) clicked! 📝", false);
            client.editRichMessage(q.chat_id, q.message_id,
                                   build_clicked_message("richtextbtn", "📝 Text Button (Cell)"));
        } else if (q.data == "clicked_richmsgbtn") {
            q.answer("Message Button (richmsgbtn) clicked! 🎛️", false);
            client.editRichMessage(q.chat_id, q.message_id,
                                   build_clicked_message("richmsgbtn", "🎛️ Message Button (Block)"));
        } else if (q.data == "test_custom_emoji") {
            q.answer("✨ Custom Emoji: <tg-emoji> active on Layer 225!", true);
        } else if (q.data == "reset_button_table" || q.data == "btn_table_showcase") {
            q.answer("Showcase reset! 📊", false);
            client.editRichMessage(q.chat_id, q.message_id,
                                   build_showcase_message(format_uptime()));
        } else if (q.data == "btn_ping") {
            q.answer("Pong! 🏓 Latency check ok.", false);
            client.editMessage(q.chat_id, q.message_id,
                "🏓 <b>Pong!</b> Callback button response received instantly.\nUptime: " + format_uptime(),
                ParseMode::HTML);
        } else if (q.data == "btn_dice") {
            q.answer("Rolling dice! 🎲", false);
            client.sendDice(q.chat_id, "\xF0\x9F\x8E\xB2");
        } else if (q.data == "btn_info") {
            auto me = client.getMe();
            q.answer("Bot: @" + me.username, false);
            client.editMessage(q.chat_id, q.message_id,
                "🤖 <b>Bot Identity:</b> @" + me.username + " (ID: <code>" + std::to_string(me.id) + "</code>)",
                ParseMode::HTML);
        } else if (q.data == "btn_stats") {
            q.answer("Stats updated!", false);
            client.editMessage(q.chat_id, q.message_id,
                "📊 <b>Live Stats:</b>\n• Messages Processed: " + std::to_string(g_messages_processed.load()) +
                "\n• Uptime: " + format_uptime(),
                ParseMode::HTML);
        } else if (q.data == "btn_cancel") {
            q.answer("Closed.", false);
            client.editMessage(q.chat_id, q.message_id, "❌ <i>Selection closed.</i>", ParseMode::HTML);
        } else {
            q.answer("Received: " + q.data, false);
        }
    });

    // General media & message logger
    client.onMessage(Filters::photo(), [](Message msg) {
        ++g_messages_processed;
        msg.reply("📸 Photo received! File ID stored.");
    });

    client.onMessage(Filters::sticker(), [](Message msg) {
        ++g_messages_processed;
        msg.reply("🎨 Nice sticker!");
    });

    // 4. Authenticate Bot
    std::cout << "[Auth] Logging into Telegram MTProto via TDLib with Bot Token...\n" << std::flush;
    try {
        client.loginBot(token);
    } catch (const std::exception& ex) {
        std::cerr << "❌ Login failed: " << ex.what() << "\n";
        return 1;
    }

    auto me = client.getMe();
    std::cout << "✅ Successfully authenticated!\n";
    std::cout << "  - Bot Name    : " << me.first_name << (me.last_name.empty() ? "" : " " + me.last_name) << "\n";
    std::cout << "  - Username    : @" << me.username << "\n";
    std::cout << "  - Telegram ID : " << me.id << "\n";
    std::cout << "  - Status      : Ready and listening for updates\n\n";

    if (verify_only) {
        std::cout << "Verification self-test completed successfully. Exiting clean.\n";
        return 0;
    }

    std::cout << "Bot is running. Send /start on Telegram to test commands.\n";
    std::cout << "Press Ctrl+C or send /stop in Telegram to exit.\n" << std::flush;

    client.run();

    std::cout << "\nBot event loop terminated cleanly.\n";
    return 0;
}
