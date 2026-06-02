// tests/phase1_smoke.cpp
// Compile-time + basic runtime tests for the CppGram public API surface.
// These do NOT connect to Telegram — they verify headers, types, and filters.
#include "cppgram/client.hpp"
#include "cppgram/errors.hpp"
#include "cppgram/filters.hpp"
#include "cppgram/handlers.hpp"
#include "cppgram/result.hpp"
#include "cppgram/media.hpp"
#include "cppgram/keyboard.hpp"
#include "cppgram/callback_query.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace cppgram;

    // ---- 1. Type sanity ----
    static_assert(sizeof(ChatId) == 8);
    static_assert(sizeof(UserId) == 8);
    static_assert(sizeof(MessageId) == 8);

    // ---- 2. Result<T> ----
    {
        Result<int> ok(42);
        assert(ok.ok());
        assert(ok.value() == 42);
        assert(ok.unwrap() == 42);

        Result<int> err(Error{ErrorCode::Network, "offline"});
        assert(!err.ok());
        bool threw = false;
        try { err.unwrap(); }
        catch (const NetworkError&) { threw = true; }
        assert(threw);
    }

    // ---- 3. Result<void> ----
    {
        Result<void> ok;
        assert(ok.ok());
        ok.unwrap();

        Result<void> err(Error{ErrorCode::Storage, "disk full"});
        assert(!err.ok());
        bool threw = false;
        try { err.unwrap(); }
        catch (const StorageError&) { threw = true; }
        assert(threw);
    }

    // ---- 4. Basic filters ----
    {
        Message msg;
        msg.text = "/ping";
        msg.chat_type = ChatType::Private;
        msg.outgoing = false;
        msg.media = MediaType::None;

        assert(Filters::command("ping")(msg));
        assert(!Filters::command("help")(msg));
        assert(Filters::privateChat()(msg));
        assert(!Filters::group()(msg));
        assert(!Filters::channel()(msg));
        assert(Filters::text()(msg));
        assert(!Filters::media()(msg));
        assert(Filters::incoming()(msg));
        assert(!Filters::outgoing()(msg));

        msg.text = "/start@mybot extra args";
        assert(Filters::command("start")(msg));
        assert(!Filters::command("stop")(msg));
    }

    // ---- 5. Filter composition ----
    {
        Message msg;
        msg.text = "hello";
        msg.chat_type = ChatType::Private;
        msg.outgoing = false;
        msg.media = MediaType::None;

        auto combined = Filters::text() && Filters::privateChat();
        assert(combined(msg));

        auto either = Filters::group() || Filters::privateChat();
        assert(either(msg));

        auto negated = !Filters::outgoing();
        assert(negated(msg));

        auto complex = Filters::text() && Filters::privateChat() && !Filters::bot();
        assert(complex(msg));
    }

    // ---- 6. Media type filters ----
    {
        Message msg;
        msg.media = MediaType::Photo;
        assert(Filters::photo()(msg));
        assert(!Filters::video()(msg));
        assert(Filters::media()(msg));

        msg.media = MediaType::Video;
        assert(Filters::video()(msg));

        msg.media = MediaType::Document;
        assert(Filters::document()(msg));

        msg.media = MediaType::Sticker;
        assert(Filters::sticker()(msg));
    }

    // ---- 7. Regex filter ----
    {
        Message msg;
        msg.text = "Hello World 123";
        assert(Filters::regex("\\d+")(msg));
        assert(Filters::regex("Hello")(msg));
        assert(!Filters::regex("^Goodbye")(msg));
    }

    // ---- 8. Chat ID / User ID filters ----
    {
        Message msg;
        msg.chat_id = 12345;
        msg.sender.id = 67890;
        assert(Filters::chatId(12345)(msg));
        assert(!Filters::chatId(99999)(msg));
        assert(Filters::userId(67890)(msg));
        assert(!Filters::userId(11111)(msg));
    }

    // ---- 9. Reply / forwarded filters ----
    {
        Message msg;
        assert(!Filters::reply()(msg));
        assert(!Filters::forwarded()(msg));

        msg.reply_to = 42;
        assert(Filters::reply()(msg));

        msg.forward_info = ForwardInfo{};
        assert(Filters::forwarded()(msg));
    }

    // ---- 10. MessageHandler struct ----
    {
        bool called = false;
        MessageHandler h{
            Filters::text(),
            [&](Message) { called = true; }
        };
        Message m;
        m.text = "hello";
        if (h.filter(m)) h.callback(m);
        assert(called);
    }

    // ---- 11. ForwardInfo / ReplyInfo types ----
    {
        ForwardInfo fi;
        fi.origin_sender_name = "test";
        assert(fi.origin_sender_id == 0);

        ReplyInfo ri;
        ri.reply_to_message_id = 123;
        assert(ri.reply_in_chat_id == 0);
    }

    // ---- 12. User::full_name ----
    {
        User u;
        u.first_name = "John";
        u.last_name = "Doe";
        assert(u.full_name() == "John Doe");

        u.last_name.clear();
        assert(u.full_name() == "John");
    }

    // ---- 13. InlineKeyboard builder ----
    {
        InlineKeyboard kb;
        kb.addButton(0, InlineKeyboardButton::callback("Click", "data1"));
        kb.addButton(0, InlineKeyboardButton::link("Open", "https://example.com"));
        kb.addButton(1, InlineKeyboardButton::callback("Row2", "data2"));
        assert(kb.rows.size() == 2);
        assert(kb.rows[0].size() == 2);
        assert(kb.rows[1].size() == 1);
        assert(kb.rows[0][0].callback_data == "data1");
        assert(kb.rows[0][1].url == "https://example.com");
    }

    // ---- 14. ReplyKeyboard builder ----
    {
        ReplyKeyboard rk;
        rk.addRow({ReplyKeyboardButton{"Option A"}, ReplyKeyboardButton{"Option B"}});
        assert(rk.rows.size() == 1);
        assert(rk.rows[0].size() == 2);
        assert(rk.resize_keyboard == true);
    }

    // ---- 15. ReplyMarkup variant ----
    {
        ReplyMarkup rm = InlineKeyboard{};
        assert(std::holds_alternative<InlineKeyboard>(rm));

        rm = RemoveKeyboard{};
        assert(std::holds_alternative<RemoveKeyboard>(rm));

        rm = ForceReply{};
        assert(std::holds_alternative<ForceReply>(rm));
    }

    // ---- 16. Photo type ----
    {
        Photo p;
        assert(p.empty());

        p.sizes.push_back(PhotoSize{1, "", 100, 100, 1000});
        p.sizes.push_back(PhotoSize{2, "", 800, 600, 50000});
        p.sizes.push_back(PhotoSize{3, "", 320, 240, 10000});
        assert(!p.empty());
        assert(p.largest().file_id == 2);
        assert(p.smallest().file_id == 1);
    }

    // ---- 17. MediaType expanded enum ----
    {
        static_assert(static_cast<int>(MediaType::VideoNote) != 0);
        static_assert(static_cast<int>(MediaType::Poll) != 0);
        static_assert(static_cast<int>(MediaType::Dice) != 0);
    }

    // ---- 18. Message convenience methods ----
    {
        Message msg;
        msg.text = "/start";
        msg.media = MediaType::None;
        assert(msg.is_command());
        assert(msg.is_text());
        assert(!msg.has_media());

        msg.media = MediaType::Photo;
        assert(!msg.is_text());
        assert(msg.has_media());
    }

    // ---- 19. ChatPermissions defaults ----
    {
        ChatPermissions p;
        assert(p.can_send_messages == true);
        assert(p.can_pin_messages == false);
        assert(p.can_manage_topics == false);
    }

    // ---- 20. ChatAdminRights defaults ----
    {
        ChatAdminRights r;
        assert(r.can_manage_chat == false);
        assert(r.can_promote_members == false);
    }

    // ---- 21. Poll types ----
    {
        PollConfig cfg;
        cfg.type = PollType::Quiz;
        cfg.correct_option_id = 2;
        cfg.explanation = "Because...";
        assert(cfg.is_anonymous == true);
    }

    // ---- 22. InputFile ----
    {
        auto f = InputFile::local("/path/to/file.jpg");
        assert(f.path == "/path/to/file.jpg");
    }

    // ---- 23. CallbackQuery struct ----
    {
        CallbackQuery q;
        q.id = 123;
        q.data = "button_1";
        q.chat_id = 456;
        assert(q.data == "button_1");
    }

    // ---- 24. FileInfo struct ----
    {
        FileInfo fi;
        fi.file_id = 42;
        fi.is_downloaded = true;
        fi.local_path = "/tmp/file.pdf";
        assert(fi.is_downloaded);
    }

    // ---- 25. Telegram API Error classification and mapping ----
    {
        // 303 Migration
        {
            Error err = Error::from_rpc(303, "PHONE_MIGRATE_2");
            assert(err.code == ErrorCode::SeeOther);
            assert(err.migrate_to_dc == 2);
            bool caught = false;
            try {
                err.raise();
            } catch (const PhoneMigrateError& e) {
                assert(e.dc() == 2);
                caught = true;
            }
            assert(caught);
        }
        {
            Error err = Error::from_rpc(303, "FILE_MIGRATE_5");
            assert(err.code == ErrorCode::SeeOther);
            assert(err.migrate_to_dc == 5);
            bool caught = false;
            try {
                err.raise();
            } catch (const FileMigrateError& e) {
                assert(e.dc() == 5);
                caught = true;
            }
            assert(caught);
        }
        {
            Error err = Error::from_rpc(303, "USER_MIGRATE_9");
            assert(err.code == ErrorCode::SeeOther);
            assert(err.migrate_to_dc == 9);
            bool caught = false;
            try {
                err.raise();
            } catch (const UserMigrateError& e) {
                assert(e.dc() == 9);
                caught = true;
            }
            assert(caught);
        }
        
        // 400 Bad Request
        {
            Error err = Error::from_rpc(400, "USERNAME_INVALID");
            assert(err.code == ErrorCode::BadRequest);
            bool caught = false;
            try {
                err.raise();
            } catch (const BadRequestError&) {
                caught = true;
            }
            assert(caught);
        }

        // 401 Unauthorized
        {
            Error err = Error::from_rpc(401, "AUTH_KEY_UNREGISTERED");
            assert(err.code == ErrorCode::Unauthorized);
            bool caught = false;
            try {
                err.raise();
            } catch (const UnauthorizedError&) {
                caught = true;
            }
            assert(caught);
        }

        // 403 Forbidden
        {
            Error err = Error::from_rpc(403, "CHAT_WRITE_FORBIDDEN");
            assert(err.code == ErrorCode::Forbidden);
            bool caught = false;
            try {
                err.raise();
            } catch (const ForbiddenError&) {
                caught = true;
            }
            assert(caught);
        }

        // 404 Not Found
        {
            Error err = Error::from_rpc(404, "CHAT_ID_INVALID");
            assert(err.code == ErrorCode::NotFound);
            bool caught = false;
            try {
                err.raise();
            } catch (const NotFoundError&) {
                caught = true;
            }
            assert(caught);
        }

        // 406 Not Acceptable
        {
            Error err = Error::from_rpc(406, "FILENAME_INVALID");
            assert(err.code == ErrorCode::NotAcceptable);
            bool caught = false;
            try {
                err.raise();
            } catch (const NotAcceptableError&) {
                caught = true;
            }
            assert(caught);
        }

        // 420 Flood / Slowmode
        {
            Error err = Error::from_rpc(420, "FLOOD_WAIT_15");
            assert(err.code == ErrorCode::FloodWait);
            assert(err.retry_after == 15);
            bool caught = false;
            try {
                err.raise();
            } catch (const FloodWaitError& e) {
                assert(e.seconds() == 15);
                assert(e.retry_after() == std::chrono::seconds(15));
                caught = true;
            }
            assert(caught);
        }
        {
            Error err = Error::from_rpc(420, "SLOWMODE_WAIT_30");
            assert(err.code == ErrorCode::Flood);
            assert(err.retry_after == 30);
            bool caught = false;
            try {
                err.raise();
            } catch (const SlowmodeWaitError& e) {
                assert(e.seconds() == 30);
                caught = true;
            }
            assert(caught);
        }

        // 500 Internal Server
        {
            Error err = Error::from_rpc(500, "RPC_CALL_FAIL");
            assert(err.code == ErrorCode::InternalServer);
            bool caught = false;
            try {
                err.raise();
            } catch (const InternalServerError&) {
                caught = true;
            }
            assert(caught);
        }
    }

    std::cout << "All smoke tests passed.\n";
    return 0;
}
