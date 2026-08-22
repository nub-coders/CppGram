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
#include "cppgram/storage.hpp"
#include "cppgram/coro.hpp"
#include "cppgram/web_app.hpp"
#include "cppgram/business.hpp"
#include "cppgram/secret_chat.hpp"
#include "cppgram/call.hpp"
#include "cppgram/transport.hpp"
#include "cppgram/crypto.hpp"
#include "cppgram/network.hpp"
#include "cppgram/session.hpp"
#include "cppgram/cli.hpp"
#include <cassert>
#include <iostream>
#include <cstdio>

using namespace cppgram;

#if __has_include(<coroutine>)
Task<int> async_add(int a, int b) {
    co_return a + b;
}

Task<int> async_compute() {
    int v1 = co_await async_add(10, 20);
    int v2 = co_await async_add(v1, 12);
    co_return v2;
}

Task<void> async_set_flag(bool& flag) {
    flag = true;
    co_return;
}

Task<int> async_throw() {
    throw std::runtime_error("coroutine error");
    co_return 0;
}
#endif

int main() {


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

    // ---- 26. ParseMode & FormattedText & MessageEntity (v0.2) ----
    {
        assert(ParseMode::None != ParseMode::Markdown);
        assert(ParseMode::Markdown != ParseMode::MarkdownV2);
        assert(ParseMode::MarkdownV2 != ParseMode::HTML);

        MessageEntity ent{MessageEntityType::Bold, 0, 5, ""};
        assert(ent.type == MessageEntityType::Bold);
        assert(ent.offset == 0);
        assert(ent.length == 5);

        MessageEntity link_ent{MessageEntityType::TextUrl, 6, 4, "https://telegram.org"};
        assert(link_ent.type == MessageEntityType::TextUrl);
        assert(link_ent.url() == "https://telegram.org");

        FormattedText ft{"Hello link", {ent, link_ent}};
        assert(ft.text == "Hello link");
        assert(ft.entities.size() == 2);
    }

    // ---- 27. SendMessageOptions (v0.2) ----
    {
        SendMessageOptions opt;
        assert(opt.parse_mode == ParseMode::None);
        assert(!opt.schedule_date.has_value());
        assert(opt.disable_notification == false);
        assert(opt.protect_content == false);

        opt.parse_mode = ParseMode::MarkdownV2;
        opt.schedule_date = std::chrono::system_clock::from_time_t(1750000000);
        opt.disable_notification = true;
        opt.protect_content = true;
        assert(opt.parse_mode == ParseMode::MarkdownV2);
        assert(opt.schedule_date.has_value() && std::chrono::system_clock::to_time_t(*opt.schedule_date) == 1750000000);
        assert(opt.disable_notification);
        assert(opt.protect_content);
    }

    // ---- 28. Session Storage (Memory & SQLite) (v0.2) ----
    {
        // Memory storage
        auto mem_store = create_storage(":memory:");
        assert(mem_store != nullptr);

        mem_store->set_value("auth_token", "abc123xyz");
        auto val = mem_store->get_value("auth_token");
        assert(val.has_value() && *val == "abc123xyz");

        PeerInfo peer{123456, 1, "7891011", "testuser", "", ""};
        mem_store->save_peer(peer);
        auto p1 = mem_store->get_peer(123456);
        assert(p1.has_value() && p1->username == "testuser" && p1->access_hash == "7891011");
        auto p2 = mem_store->get_peer_by_username("testuser");
        assert(p2.has_value() && p2->id == 123456);

        mem_store->delete_value("auth_token");
        assert(!mem_store->get_value("auth_token").has_value());

        // SQLite disk storage
        const std::string test_db = "/tmp/test_cppgram_smoke.db";
        std::remove(test_db.c_str());

        auto sql_store = create_storage(test_db);
        assert(sql_store != nullptr);

        sql_store->set_value("key1", "value1");
        auto sval = sql_store->get_value("key1");
        assert(sval.has_value() && *sval == "value1");

        PeerInfo sql_peer{-100123456789, 3, "999888", "mychannel", "", ""};
        sql_store->save_peer(sql_peer);
        auto sp1 = sql_store->get_peer(-100123456789);
        assert(sp1.has_value() && sp1->username == "mychannel" && sp1->type == 3);
        auto sp2 = sql_store->get_peer_by_username("mychannel");
        assert(sp2.has_value() && sp2->id == -100123456789);

        sql_store->delete_value("key1");
        assert(!sql_store->get_value("key1").has_value());

        std::remove(test_db.c_str());
    }

    // ---- 29. UserProfilePhotos (v0.2) ----
    {
        UserProfilePhotos photos;
        photos.total_count = 2;
        Photo p1, p2;
        p1.sizes.push_back(PhotoSize{1, "", 160, 160, 5000});
        p2.sizes.push_back(PhotoSize{2, "", 640, 640, 25000});
        photos.photos.push_back(p1);
        photos.photos.push_back(p2);

        assert(photos.total_count == 2);
        assert(photos.photos.size() == 2);
        assert(photos.photos[0].sizes[0].width == 160);
        assert(photos.photos[1].sizes[0].width == 640);
    }

    // ---- 30. C++20 Coroutines (v0.2) ----
#if __has_include(<coroutine>)
    {
        int sum = sync_wait(async_compute());
        assert(sum == 42);

        bool flag = false;
        sync_wait(async_set_flag(flag));
        assert(flag == true);

        bool threw = false;
        try {
            sync_wait(async_throw());
        } catch (const std::runtime_error& e) {
            threw = true;
            assert(std::string(e.what()) == "coroutine error");
        }
        assert(threw);
    }
#endif

    // ---- 31. ThreadPool (v0.3) ----
    {
        ThreadPool pool(4);
        assert(pool.size() == 4);
        assert(pool.is_running());

        std::atomic<int> counter{0};
        constexpr int kTasks = 50;
        for (int i = 0; i < kTasks; ++i) {
            pool.enqueue([&counter] {
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
        pool.wait_all();
        assert(counter.load() == kTasks);
        assert(pool.completed_tasks() == kTasks);

        pool.shutdown();
        assert(!pool.is_running());
    }

    // ---- 32. Plugin Architecture (v0.3) ----
    {
        class TestPlugin : public IPlugin {
        public:
            bool loaded{false};
            bool unloaded{false};
            std::string name() const override { return "TestPlugin"; }
            std::string version() const override { return "1.2.3"; }
            std::string description() const override { return "Test Description"; }
            void on_load(Client&) override { loaded = true; }
            void on_unload(Client&) override { unloaded = true; }
        };

        PluginManager mgr;
        Client dummy_client;
        auto plugin = std::make_shared<TestPlugin>();

        assert(!mgr.has_plugin("TestPlugin"));
        assert(mgr.list_plugins().empty());

        bool ok = mgr.register_plugin(plugin, dummy_client);
        assert(ok);
        assert(plugin->loaded);
        assert(mgr.has_plugin("TestPlugin"));
        assert(mgr.get_plugin("TestPlugin") == plugin);
        assert(mgr.list_plugins().size() == 1);

        // Duplicate registration must fail
        assert(!mgr.register_plugin(plugin, dummy_client));

        // Unregister
        bool unreg = mgr.unregister_plugin("TestPlugin", dummy_client);
        assert(unreg);
        assert(plugin->unloaded);
        assert(!mgr.has_plugin("TestPlugin"));
        assert(mgr.list_plugins().empty());

        // Unregister non-existent
        assert(!mgr.unregister_plugin("Unknown", dummy_client));

        // Test unload_all
        auto p2 = std::make_shared<TestPlugin>();
        mgr.register_plugin(p2, dummy_client);
        assert(mgr.has_plugin("TestPlugin"));
        mgr.unload_all(dummy_client);
        assert(p2->unloaded);
        assert(!mgr.has_plugin("TestPlugin"));
    }

    // ---- 33. Middleware Pipeline (v0.3) ----
    {
        MiddlewarePipeline pipeline;
        assert(pipeline.empty());

        std::vector<std::string> log;

        pipeline.use([&log](MiddlewareContext& ctx) -> bool {
            log.push_back("m1_pre");
            ctx.set_data("trace_id", "xyz-123");
            return true;
        });

        pipeline.use([&log](MiddlewareContext& ctx) -> bool {
            log.push_back("m2_pre");
            auto trace = ctx.get_data("trace_id");
            assert(trace.has_value() && *trace == "xyz-123");
            return true;
        });

        assert(pipeline.size() == 2);

        Client dummy_client;
        Message msg;
        msg.id = 100;
        msg.chat_id = 200;
        msg.text = "hello middleware";

        MiddlewareContext ctx(dummy_client, msg, std::nullopt);
        bool executed = pipeline.execute(ctx);
        assert(executed);
        assert(log.size() == 2);
        assert(log[0] == "m1_pre");
        assert(log[1] == "m2_pre");

        // Test halting / short-circuiting
        MiddlewarePipeline halt_pipeline;
        halt_pipeline.use([](MiddlewareContext&) -> bool {
            return false; // drop update
        });
        halt_pipeline.use([&log](MiddlewareContext&) -> bool {
            log.push_back("should_not_run");
            return true;
        });
        MiddlewareContext ctx2(dummy_client, msg, std::nullopt);
        assert(!halt_pipeline.execute(ctx2));
        assert(log.size() == 2); // didn't run the second middleware
    }

    // ---- 34. Message Threads & Topics (v0.3) ----
    {
        MessageThreadInfo info;
        info.chat_id = -1001234567890LL;
        info.message_thread_id = 42;
        info.unread_message_count = 5;
        info.messages.push_back(Message{});
        info.messages[0].id = 101;
        info.messages[0].message_thread_id = 42;

        assert(info.chat_id == -1001234567890LL);
        assert(info.message_thread_id == 42);
        assert(info.unread_message_count == 5);
        assert(info.messages.size() == 1);
        assert(info.messages[0].message_thread_id.has_value() && *info.messages[0].message_thread_id == 42);

        // Filters::thread and Filters::in_thread
        Message msg_thread;
        msg_thread.id = 1;
        msg_thread.message_thread_id = 42;
        assert(Filters::thread(42)(msg_thread));
        assert(!Filters::thread(99)(msg_thread));
        assert(Filters::in_thread()(msg_thread));

        Message msg_nothread;
        msg_nothread.id = 2;
        assert(!Filters::thread(42)(msg_nothread));
        assert(!Filters::in_thread()(msg_nothread));

        // ForumTopic structure
        ForumTopic topic;
        topic.message_thread_id = 77;
        topic.name = "General Discussion";
        topic.icon_color = 0x6FB9F0;
        topic.is_closed = false;
        topic.is_hidden = false;
        assert(topic.message_thread_id == 77);
        assert(topic.name == "General Discussion");
        assert(!topic.is_closed);
        assert(!topic.is_hidden);

        // SendMessageOptions with message_thread_id
        SendMessageOptions opts;
        opts.message_thread_id = 77;
        assert(opts.message_thread_id.has_value() && *opts.message_thread_id == 77);
    }

    // ---- 35. Telegram Stories (v0.3) ----
    {
        StoryItem story;
        story.id = 12345;
        story.sender_chat_id = -1009876543210LL;
        story.date = std::chrono::system_clock::from_time_t(1750000000);
        story.is_pinned = true;
        story.caption = "Holiday trip!";

        assert(story.id == 12345);
        assert(story.sender_chat_id == -1009876543210LL);
        assert(story.is_pinned);
        assert(story.caption == "Holiday trip!");

        StoryPrivacySettings privacy;
        privacy.privacy = StoryPrivacy::Public;
        assert(privacy.privacy == StoryPrivacy::Public);

        Stories stories;
        stories.total_count = 1;
        stories.stories.push_back(story);
        assert(stories.total_count == 1);
        assert(stories.stories.size() == 1);
        assert(stories.stories[0].id == 12345);
    }

    // ---- 36. Telegram Mini Apps & Web Apps (v0.4) ----
    {
        WebAppInfo app{"https://webapp.telegram.org"};
        assert(app.url == "https://webapp.telegram.org");

        WebAppData data{"{\"status\":\"success\",\"tx_id\":987}", "Pay Now"};
        assert(data.data == "{\"status\":\"success\",\"tx_id\":987}");
        assert(data.button_text == "Pay Now");

        SentWebAppMessage sent{"inline_msg_777"};
        assert(sent.inline_message_id == "inline_msg_777");

        Message msg_webapp;
        msg_webapp.web_app_data = data;
        assert(Filters::webAppData()(msg_webapp));

        Message msg_no_webapp;
        assert(!Filters::webAppData()(msg_no_webapp));

        InlineKeyboardButton btn_webapp = InlineKeyboardButton::web_app("Open Mini App", "https://webapp.telegram.org");
        assert(btn_webapp.text == "Open Mini App" && btn_webapp.url == "https://webapp.telegram.org");

        ReplyKeyboardButton rbtn_webapp = ReplyKeyboardButton::web_app("Open App", "https://webapp.telegram.org");
        assert(rbtn_webapp.text == "Open App" && rbtn_webapp.web_app_url == "https://webapp.telegram.org");
    }

    // ---- 37. Telegram Business Bots (v0.4) ----
    {
        BusinessConnection bconn;
        bconn.id = "biz_conn_12345";
        bconn.user.id = 987654321;
        bconn.user_chat_id = 123456789;
        bconn.date = std::chrono::system_clock::now();
        bconn.can_reply = true;
        bconn.is_enabled = true;
        assert(bconn.id == "biz_conn_12345");
        assert(bconn.user.id == 987654321);
        assert(bconn.can_reply && bconn.is_enabled);

        BusinessIntro intro;
        intro.title = "Welcome to Our Store!";
        intro.message = "Open 24/7 for fast delivery.";
        assert(intro.title == "Welcome to Our Store!");
        assert(intro.message == "Open 24/7 for fast delivery.");

        BusinessLocation bloc;
        bloc.address = "742 Evergreen Terrace";
        bloc.location = Location{40.7128, -74.0060};
        assert(bloc.address == "742 Evergreen Terrace");
        assert(bloc.location.has_value() && bloc.location->latitude == 40.7128);

        BusinessOpeningHoursInterval interval{480, 1080};
        BusinessOpeningHours bhours;
        bhours.time_zone_name = "America/New_York";
        bhours.opening_hours.push_back(interval);
        assert(bhours.time_zone_name == "America/New_York");
        assert(bhours.opening_hours.size() == 1);
        assert(bhours.opening_hours[0].opening_minute == 480);

        QuickReplyShortcut shortcut;
        shortcut.id = 101;
        shortcut.name = "help";
        shortcut.message_count = 3;
        assert(shortcut.id == 101);
        assert(shortcut.name == "help");
        assert(shortcut.message_count == 3);

        Message msg_biz;
        msg_biz.business_connection_id = "biz_conn_12345";
        assert(Filters::business()(msg_biz));
        assert(Filters::businessConnectionId("biz_conn_12345")(msg_biz));
        assert(!Filters::businessConnectionId("other_conn")(msg_biz));

        Message msg_nobiz;
        assert(!Filters::business()(msg_nobiz));
        assert(!Filters::businessConnectionId("biz_conn_12345")(msg_nobiz));
    }

    // ---- 38. Secret Chats & E2EE Models (v0.4) ----
    {
        SecretChat sc;
        sc.id = 789;
        sc.user_id = 456;
        sc.state = SecretChatState::Ready;
        sc.is_outbound = true;
        sc.layer = 144;
        sc.key_hash = "aabbccddeeff0011";
        assert(sc.id == 789);
        assert(sc.user_id == 456);
        assert(sc.state == SecretChatState::Ready);
        assert(sc.is_outbound);
        assert(sc.layer == 144);
        assert(sc.key_hash == "aabbccddeeff0011");

        SecretChat sc_pending;
        sc_pending.state = SecretChatState::Pending;
        assert(sc_pending.state == SecretChatState::Pending);

        SecretChat sc_closed;
        sc_closed.state = SecretChatState::Closed;
        assert(sc_closed.state == SecretChatState::Closed);
    }

    // ---- 39. Voice & Group Calls Models (v0.4) ----
    {
        GroupCall gc;
        gc.id = 3001;
        gc.title = "Weekly Core Team Sync";
        gc.duration = 3600;
        gc.is_active = true;
        gc.is_joined = true;
        gc.participant_count = 8;
        gc.loaded_all_participants = true;
        assert(gc.id == 3001);
        assert(gc.title == "Weekly Core Team Sync");
        assert(gc.duration == 3600);
        assert(gc.is_active && gc.is_joined);
        assert(gc.participant_count == 8);

        GroupCallParticipant p;
        p.user_id = 999;
        p.is_muted = false;
        p.is_speaking = true;
        p.volume_level = 150;
        p.order = "participant_01";
        assert(p.user_id == 999);
        assert(!p.is_muted && p.is_speaking);
        assert(p.volume_level == 150);

        CallProtocol proto;
        proto.udp_p2p = true;
        proto.udp_reflector = true;
        proto.min_layer = 65;
        proto.max_layer = 92;
        proto.library_versions = {"2.4.0", "2.4.1"};
        assert(proto.udp_p2p && proto.udp_reflector);
        assert(proto.min_layer == 65 && proto.max_layer == 92);
        assert(proto.library_versions.size() == 2);

        CallServer srv;
        srv.id = 1;
        srv.ip = "149.154.167.50";
        srv.ipv6 = "2001:67c:4e8:f002::a";
        srv.port = 443;
        srv.type = "webrtc";
        assert(srv.id == 1);
        assert(srv.ip == "149.154.167.50");
        assert(srv.port == 443);
    }

    // ---- 40. Standalone MTProto Transport Packet Codecs (v0.4) ----
    {
        // CRC32 calculation check
        std::vector<uint8_t> crc_test_data = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        uint32_t crc = FullCodec::compute_crc32(crc_test_data.data(), crc_test_data.size());
        assert(crc == 0xCBF43926); // Standard IEEE 802.3 CRC32 check value

        // 1. Abridged Codec
        {
            auto codec = create_transport_codec(TransportProtocol::Abridged);
            assert(codec != nullptr);
            auto header = codec->get_header();
            assert(header.size() == 1 && header[0] == 0xef);

            // Encode small payload (16 bytes = 4 words of 4 bytes)
            std::vector<uint8_t> payload = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
            auto packet = codec->encode_packet(payload);
            assert(packet.size() == 17);
            assert(packet[0] == 4); // 16 / 4 = 4

            // Feed partial bytes and decode
            auto pkts1 = codec->decode_packets(packet.data(), 10);
            assert(pkts1.empty());
            auto pkts2 = codec->decode_packets(packet.data() + 10, 7);
            assert(pkts2.size() == 1 && pkts2[0] == payload);

            // Encode large payload (> 127 words, e.g., 600 bytes = 150 words)
            std::vector<uint8_t> large_payload(600, 0x42);
            auto large_packet = codec->encode_packet(large_payload);
            assert(large_packet.size() == 4 + 600);
            assert(large_packet[0] == 0x7f);

            auto large_pkts = codec->decode_packets(large_packet);
            assert(large_pkts.size() == 1 && large_pkts[0] == large_payload);
        }

        // 2. Intermediate Codec
        {
            auto codec = create_transport_codec(TransportProtocol::Intermediate);
            assert(codec != nullptr);
            auto header = codec->get_header();
            assert(header.size() == 4);
            assert(header[0] == 0xee && header[1] == 0xee && header[2] == 0xee && header[3] == 0xee);

            std::vector<uint8_t> payload = {10, 20, 30, 40, 50, 60, 70, 80};
            auto packet = codec->encode_packet(payload);
            assert(packet.size() == 4 + 8);

            // Feed and decode
            auto pkts = codec->decode_packets(packet);
            assert(pkts.size() == 1 && pkts[0] == payload);
        }

        // 3. Full Codec
        {
            auto codec = create_transport_codec(TransportProtocol::Full);
            assert(codec != nullptr);
            assert(codec->get_header().empty());

            std::vector<uint8_t> payload = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
            auto packet0 = codec->encode_packet(payload);
            // Full codec packet: 4 bytes len (payload + 12) + 4 bytes seq (0) + 11 bytes payload + 4 bytes CRC
            assert(packet0.size() == 4 + 4 + 11 + 4);

            auto packet1 = codec->encode_packet(payload);
            // Full codec packet: seq should be 1
            assert(packet1.size() == packet0.size());

            // Feed packet 0
            auto pkts0 = codec->decode_packets(packet0);
            assert(pkts0.size() == 1 && pkts0[0] == payload);

            // Feed packet 1 in fragments
            auto f1 = codec->decode_packets(packet1.data(), 5);
            assert(f1.empty());
            auto f2 = codec->decode_packets(packet1.data() + 5, packet1.size() - 5);
            assert(f2.size() == 1 && f2[0] == payload);

            // Test corrupted CRC detection
            std::vector<uint8_t> bad_packet = packet0;
            bad_packet.back() ^= 0xFF; // corrupt CRC byte
            bool threw = false;
            try {
                codec->decode_packets(bad_packet);
            } catch (const std::runtime_error&) {
                threw = true;
            }
            assert(threw);
        }
    }

    // ---- 41. MTProto 2.0 SHA-256 & SHA-1 (v0.5) ----
    {
        std::vector<uint8_t> empty_data;
        auto sha256_empty = CryptoUtils::compute_sha256(empty_data);
        assert(sha256_empty.size() == 32);
        // Known SHA-256 of empty string: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
        assert(sha256_empty[0] == 0xe3 && sha256_empty[1] == 0xb0 && sha256_empty[31] == 0x55);

        std::vector<uint8_t> test_data = {'t', 'e', 's', 't'};
        auto sha1_test = CryptoUtils::compute_sha1(test_data);
        assert(sha1_test.size() == 20);
        // Known SHA-1 of "test": a94a8fe5ccb19ba61c4c0873d391e987982fbbd3
        assert(sha1_test[0] == 0xa9 && sha1_test[1] == 0x4a && sha1_test[19] == 0xd3);
    }

    // ---- 42. AES-256-IGE Mode Encryption & Decryption (v0.5) ----
    {
        std::vector<uint8_t> key(32, 0x11);
        std::vector<uint8_t> iv(32, 0x22);
        std::vector<uint8_t> plaintext(64);
        for (size_t i = 0; i < plaintext.size(); ++i) {
            plaintext[i] = static_cast<uint8_t>(i * 3 + 1);
        }

        auto ciphertext = CryptoUtils::aes_ige_encrypt(plaintext, key, iv);
        assert(ciphertext.size() == plaintext.size());
        assert(ciphertext != plaintext);

        auto decrypted = CryptoUtils::aes_ige_decrypt(ciphertext, key, iv);
        assert(decrypted == plaintext);

        // Invalid key size check
        bool threw_key = false;
        try {
            CryptoUtils::aes_ige_encrypt(plaintext, std::vector<uint8_t>(16), iv);
        } catch (const std::invalid_argument&) {
            threw_key = true;
        }
        assert(threw_key);

        // Invalid plaintext size check (not multiple of 16)
        bool threw_align = false;
        try {
            CryptoUtils::aes_ige_encrypt(std::vector<uint8_t>(15), key, iv);
        } catch (const std::invalid_argument&) {
            threw_align = true;
        }
        assert(threw_align);
    }

    // ---- 43. AES-256-CTR Mode Encryption & Decryption (v0.5) ----
    {
        std::vector<uint8_t> key(32, 0x33);
        std::vector<uint8_t> iv(16, 0x44);
        std::vector<uint8_t> message = {'c', 'p', 'p', 'g', 'r', 'a', 'm', ' ', 'c', 't', 'r'};

        auto enc = CryptoUtils::aes_ctr_encrypt(message, key, iv);
        assert(enc.size() == message.size());
        assert(enc != message);

        auto dec = CryptoUtils::aes_ctr_decrypt(enc, key, iv);
        assert(dec == message);
    }

    // ---- 44. MTProto 2.0 Key Derivation Function (KDF) (v0.5) ----
    {
        std::vector<uint8_t> auth_key(256);
        for (size_t i = 0; i < 256; ++i) {
            auth_key[i] = static_cast<uint8_t>(i ^ 0x55);
        }
        std::vector<uint8_t> msg_key(16, 0x66);

        std::vector<uint8_t> client_aes_key, client_aes_iv;
        bool ok_client = CryptoUtils::kdf_mtproto2(auth_key, msg_key, true, client_aes_key, client_aes_iv);
        assert(ok_client);
        assert(client_aes_key.size() == 32);
        assert(client_aes_iv.size() == 32);

        std::vector<uint8_t> server_aes_key, server_aes_iv;
        bool ok_server = CryptoUtils::kdf_mtproto2(auth_key, msg_key, false, server_aes_key, server_aes_iv);
        assert(ok_server);
        assert(server_aes_key.size() == 32);
        assert(server_aes_iv.size() == 32);

        // Client and Server keys should differ because x offset differs (0 vs 8)
        assert(client_aes_key != server_aes_key);
        assert(client_aes_iv != server_aes_iv);
    }

    // ---- 45. MTProto 2.0 msg_key & AuthKey ID (v0.5) ----
    {
        std::vector<uint8_t> auth_key(256);
        for (size_t i = 0; i < 256; ++i) {
            auth_key[i] = static_cast<uint8_t>((i * 17) & 0xFF);
        }
        std::vector<uint8_t> payload = {1, 2, 3, 4, 5, 6, 7, 8};

        auto msg_key_client = CryptoUtils::compute_msg_key(auth_key, payload, true);
        assert(msg_key_client.size() == 16);

        auto msg_key_server = CryptoUtils::compute_msg_key(auth_key, payload, false);
        assert(msg_key_server.size() == 16);
        assert(msg_key_client != msg_key_server);

        uint64_t key_id = CryptoUtils::compute_auth_key_id(auth_key);
        assert(key_id != 0);

        auto rand_bytes = CryptoUtils::generate_random_bytes(32);
        assert(rand_bytes.size() == 32);
    }

    // ---- 46. DatacenterManager & DC Routing (v0.5) ----
    {
        DatacenterManager dcm;
        auto primary = dcm.get_primary_dc();
        assert(primary != nullptr);
        assert(primary->id == 2); // DC 2 Amsterdam is default
        assert(primary->ip_v4 == "149.154.167.51");
        assert(primary->port == 443);

        auto dc1 = dcm.get_dc(1);
        assert(dc1 != nullptr && dc1->ip_v4 == "149.154.175.53");

        auto dc5 = dcm.get_dc(5);
        assert(dc5 != nullptr && dc5->ip_v4 == "91.108.56.130");

        auto test_dc1 = dcm.get_dc(1, true);
        assert(test_dc1 != nullptr && test_dc1->is_test);
        assert(test_dc1->ip_v4 == "149.154.175.10");

        dcm.set_primary_dc(4);
        assert(dcm.get_primary_dc_id() == 4);
        assert(dcm.get_primary_dc()->ip_v4 == "149.154.167.91");

        // Custom DC registration
        DataCenter custom_dc{99, "127.0.0.1", "::1", 8443, false, "Local Mock DC"};
        dcm.register_custom_dc(custom_dc);
        auto found = dcm.get_dc(99);
        assert(found != nullptr && found->name == "Local Mock DC" && found->port == 8443);

        auto all_prods = dcm.get_all_dcs(false);
        assert(all_prods.size() >= 5);
    }

    // ---- 47. MTProto Session ID & Monotonic Message ID (v0.5) ----
    {
        Session session;
        assert(session.get_session_id() != 0);

        session.set_server_salt(0x1234567890ABCDEFULL);
        assert(session.get_server_salt() == 0x1234567890ABCDEFULL);

        session.set_time_offset(50);
        assert(session.get_time_offset() == 50);

        int64_t id1 = session.generate_msg_id();
        int64_t id2 = session.generate_msg_id();
        int64_t id3 = session.generate_msg_id();
        assert(id1 < id2);
        assert(id2 < id3);
        assert((id1 & 3) == 0); // Client message lower 2 bits must be 0

        int32_t s1 = session.generate_seq_no(true);  // content-related -> 1
        int32_t s2 = session.generate_seq_no(false); // non-content -> 2
        int32_t s3 = session.generate_seq_no(true);  // content-related -> 3
        assert(s1 == 1);
        assert(s2 == 2);
        assert(s3 == 3);
    }

    // ---- 48. Session Unencrypted Messages (v0.5) ----
    {
        Session session;
        std::vector<uint8_t> raw_payload = {'r', 'e', 'q', '_', 'p', 'q'};
        auto packed = session.pack_unencrypted_message(raw_payload);
        assert(packed.size() == 20 + raw_payload.size());

        int64_t parsed_msg_id = 0;
        std::vector<uint8_t> unpacked_payload;
        bool ok = Session::unpack_unencrypted_message(packed, parsed_msg_id, unpacked_payload);
        assert(ok);
        assert(parsed_msg_id != 0);
        assert(unpacked_payload == raw_payload);

        // Malformed unencrypted packet
        std::vector<uint8_t> bad_packet = {0, 0, 0};
        assert(!Session::unpack_unencrypted_message(bad_packet, parsed_msg_id, unpacked_payload));
    }

    // ---- 49. Session Encrypted Messages (MTProto 2.0) (v0.5) ----
    {
        std::vector<uint8_t> auth_key(256);
        for (size_t i = 0; i < 256; ++i) {
            auth_key[i] = static_cast<uint8_t>((i * 7 + 13) & 0xFF);
        }

        Session sender(0xAAAAAAAA11112222ULL);
        sender.set_auth_key(auth_key);
        sender.set_server_salt(0xBBBBBBBB33334444ULL);

        std::vector<uint8_t> payload = {'g', 'e', 't', '_', 'u', 's', 'e', 'r', '_', 'i', 'n', 'f', 'o'};
        auto encrypted_packet = sender.pack_encrypted_message(payload, true);
        assert(encrypted_packet.size() >= 24 + 32);

        // Receiver unpacks with same auth_key
        Session receiver;
        receiver.set_auth_key(auth_key);

        int64_t rx_msg_id = 0;
        int32_t rx_seq_no = 0;
        std::vector<uint8_t> rx_payload;

        bool ok = receiver.unpack_encrypted_message(encrypted_packet, rx_msg_id, rx_seq_no, rx_payload);
        assert(ok);
        assert(rx_payload == payload);
        assert(rx_msg_id != 0);
        assert(rx_seq_no == 1);
    }

    // ---- 50. Interactive CLI & Shell Commands (v0.5) ----
    {
        InteractiveCLI cli;
        assert(cli.has_command("/help"));
        assert(cli.has_command("/clear"));
        assert(cli.has_command("/exit"));
        assert(cli.has_command("/quit"));

        bool custom_ran = false;
        std::string received_arg;

        cli.register_command(
            "/send",
            "Send a message to a chat",
            "/send <chat_id> <text>",
            [&](const CommandContext& ctx) {
                if (ctx.args.size() >= 2) {
                    custom_ran = true;
                    received_arg = ctx.args[1];
                    ctx.out << "Sent to " << ctx.args[0] << ": " << ctx.args[1] << "\n";
                }
            });

        assert(cli.has_command("/send"));

        std::ostringstream oss;
        bool cont = cli.execute_line("/send 12345 \"Hello CppGram v0.5!\"", oss);
        assert(cont == true);
        assert(custom_ran == true);
        assert(received_arg == "Hello CppGram v0.5!");
        assert(oss.str().find("Sent to 12345: Hello CppGram v0.5!") != std::string::npos);

        // Test help execution
        std::ostringstream help_oss;
        cli.execute_line("/help", help_oss);
        assert(help_oss.str().find("Available Commands") != std::string::npos);
        assert(help_oss.str().find("/send") != std::string::npos);

        // Test exit command
        std::ostringstream exit_oss;
        bool cont_exit = cli.execute_line("/exit", exit_oss);
        assert(cont_exit == false);
    }

    std::cout << "All smoke tests passed (including v0.1, v0.2, v0.3, v0.4, and v0.5 features).\n";
    return 0;
}
