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

    std::cout << "All smoke tests passed (including v0.1, v0.2, and v0.3 features).\n";
    return 0;
}
