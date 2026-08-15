// examples/v03_features_demo.cpp — CppGram Version 0.3 Features Demonstration.
// Demonstrates:
// 1. Modular Plugin Architecture (IPlugin, PluginManager, loadPlugin, unloadPlugin)
// 2. Middleware Pipeline (MiddlewarePipeline, MiddlewareContext, interceptors, early halt)
// 3. Message Threading & Forum Topics (MessageThreadInfo, ForumTopic, Filters::thread, reply_in_thread)
// 4. Telegram Stories Domain Models (StoryItem, StoryPrivacySettings, Stories)
// 5. Worker Thread Pool & Dispatch Concurrency (ThreadPool, setThreadPoolSize)

#include "cppgram/client.hpp"
#include "cppgram/filters.hpp"
#include "cppgram/plugin.hpp"
#include "cppgram/middleware.hpp"
#include "cppgram/thread.hpp"
#include "cppgram/story.hpp"
#include "cppgram/thread_pool.hpp"
#include "cppgram/log.hpp"
#include <cstdlib>
#include <iostream>
#include <chrono>

using namespace cppgram;

// ---------------------------------------------------------------------------
// 1. Modular Plugin Implementation
// ---------------------------------------------------------------------------
class PingStatsPlugin : public IPlugin {
public:
    std::string name() const override { return "PingStats"; }
    std::string version() const override { return "1.0.0"; }
    std::string description() const override { return "Handles /ping and tracks execution stats"; }

    void on_load(Client& client) override {
        std::cout << "[Plugin:PingStats] Loading plugin into client...\n";
        client.onMessage(Filters::command("ping"), [this](Message msg) {
            ++ping_count_;
            std::cout << "[Plugin:PingStats] Received /ping (total: " << ping_count_ << ")\n";
            msg.reply("🏓 <b>Pong!</b> (Ping count: " + std::to_string(ping_count_) + ")", ParseMode::HTML);
        });
    }

    void on_unload(Client& client) override {
        (void)client;
        std::cout << "[Plugin:PingStats] Unloading plugin. Total pings handled: " << ping_count_ << "\n";
    }

    size_t get_ping_count() const { return ping_count_; }

private:
    size_t ping_count_{0};
};

class TopicModeratorPlugin : public IPlugin {
public:
    std::string name() const override { return "TopicModerator"; }
    std::string version() const override { return "1.0.0"; }
    std::string description() const override { return "Thread-aware message logger and auto-responder"; }

    void on_load(Client& client) override {
        std::cout << "[Plugin:TopicModerator] Registering thread filters...\n";

        // Listen for messages strictly within forum topics or reply threads
        client.onMessage(Filters::in_thread(), [](Message msg) {
            int64_t tid = msg.message_thread_id.value_or(0);
            std::cout << "[Plugin:TopicModerator] Thread message received in thread #" << tid
                      << ": " << msg.text << "\n";
            msg.reply_in_thread("💬 <i>Logged in thread #" + std::to_string(tid) + "</i>", ParseMode::HTML);
        });
    }

    void on_unload(Client&) override {
        std::cout << "[Plugin:TopicModerator] Unloaded.\n";
    }
};

int main() {
    Logger::instance().set_level(LogLevel::Info);

    const char* id_str = std::getenv("CPPGRAM_API_ID");
    const char* hash   = std::getenv("CPPGRAM_API_HASH");
    const char* token  = std::getenv("CPPGRAM_BOT_TOKEN");

    if (!id_str || !hash || !token) {
        std::cout << "=== CppGram Version 0.3 Features Showcase (Offline Mode) ===\n\n";

        // ---- 1. ThreadPool Demo ----
        std::cout << "--- 1. Worker ThreadPool & Concurrency Demo ---\n";
        ThreadPool pool(4);
        std::cout << "Initialized ThreadPool with " << pool.size() << " worker threads.\n";

        std::atomic<int> processed{0};
        for (int i = 1; i <= 8; ++i) {
            pool.enqueue([i, &processed] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                processed.fetch_add(1, std::memory_order_relaxed);
                std::cout << "  Worker executed task #" << i << "\n";
            });
        }
        pool.wait_all();
        std::cout << "All tasks finished. Total completed: " << pool.completed_tasks() << "\n\n";

        // ---- 2. Middleware Pipeline Demo ----
        std::cout << "--- 2. Middleware Pipeline Demo ---\n";
        MiddlewarePipeline pipeline;

        // Logging middleware
        pipeline.use([](MiddlewareContext& ctx) -> bool {
            ctx.set_data("request_start", "2026-09-04T12:00:00Z");
            std::cout << "  [Middleware 1] Attached request timestamp.\n";
            return true;
        });

        // Security / Filter middleware
        pipeline.use([](MiddlewareContext& ctx) -> bool {
            if (ctx.is_message()) {
                auto& msg = ctx.message();
                if (msg && msg->text.find("SPAM") != std::string::npos) {
                    std::cout << "  [Middleware 2] Blocked spam message!\n";
                    return false; // halt pipeline
                }
            }
            std::cout << "  [Middleware 2] Message passed security check.\n";
            return true;
        });

        Client dummy_client;
        Message valid_msg;
        valid_msg.text = "Hello from CppGram v0.3!";
        MiddlewareContext valid_ctx(dummy_client, valid_msg, std::nullopt);
        std::cout << "Executing pipeline for valid message...\n";
        bool passed = pipeline.execute(valid_ctx);
        std::cout << "Pipeline result: " << (passed ? "PASSED" : "HALTED") << "\n\n";

        Message spam_msg;
        spam_msg.text = "BUY NOW SPAM LINK";
        MiddlewareContext spam_ctx(dummy_client, spam_msg, std::nullopt);
        std::cout << "Executing pipeline for spam message...\n";
        bool spam_passed = pipeline.execute(spam_ctx);
        std::cout << "Pipeline result: " << (spam_passed ? "PASSED" : "HALTED") << "\n\n";

        // ---- 3. Plugin Architecture Demo ----
        std::cout << "--- 3. Plugin Architecture Demo ---\n";
        PluginManager plugin_mgr;
        auto ping_plugin = std::make_shared<PingStatsPlugin>();
        auto topic_plugin = std::make_shared<TopicModeratorPlugin>();

        plugin_mgr.register_plugin(ping_plugin, dummy_client);
        plugin_mgr.register_plugin(topic_plugin, dummy_client);

        std::cout << "Registered plugins (" << plugin_mgr.list_plugins().size() << " total):\n";
        for (const auto& p : plugin_mgr.list_plugins()) {
            std::cout << "  • " << p->name() << " v" << p->version() << " — " << p->description() << "\n";
        }

        plugin_mgr.unload_all(dummy_client);
        std::cout << "Unloaded all plugins successfully.\n\n";

        // ---- 4. Message Threading & Stories Models Demo ----
        std::cout << "--- 4. Message Threading & Stories Models Demo ---\n";
        MessageThreadInfo thread_info;
        thread_info.chat_id = -1001234567890LL;
        thread_info.message_thread_id = 42;
        thread_info.unread_message_count = 3;
        std::cout << "Created MessageThreadInfo for Chat: " << thread_info.chat_id 
                  << ", Thread ID: " << thread_info.message_thread_id 
                  << ", Unread: " << thread_info.unread_message_count << "\n";

        StoryItem story;
        story.id = 999;
        story.sender_chat_id = -1009876543210LL;
        story.caption = "Exploring Telegram Stories in CppGram v0.3!";
        story.date = std::chrono::system_clock::now();
        std::cout << "Created StoryItem ID: " << story.id << " with caption: \"" << story.caption << "\"\n\n";

        std::cout << "To run live against Telegram, set CPPGRAM_API_ID, CPPGRAM_API_HASH, and CPPGRAM_BOT_TOKEN.\n";
        return 0;
    }

    // ---- Live Telegram Client with v0.3 Features ----
    std::cout << "Connecting to Telegram Bot with v0.3 features enabled...\n";
    Client client(std::atoi(id_str), hash);
    client.setDefaultParseMode(ParseMode::HTML);

    // Set concurrent worker thread pool size
    client.setThreadPoolSize(4);
    std::cout << "Thread pool workers: " << client.getThreadPoolSize() << "\n";

    // Register global logging middleware
    client.use([](MiddlewareContext& ctx) -> bool {
        if (ctx.is_message()) {
            auto& m = ctx.message();
            if (m) {
                std::cout << "[Middleware] Incoming message from chat " << m->chat_id 
                          << ": " << m->text << "\n";
            }
        }
        return true;
    });

    // Register plugins
    auto ping_plugin = std::make_shared<PingStatsPlugin>();
    auto topic_plugin = std::make_shared<TopicModeratorPlugin>();
    client.loadPlugin(ping_plugin);
    client.loadPlugin(topic_plugin);

    client.loginBot(token);
    auto me = client.getMe();
    std::cout << "Bot @" << me.username << " is online with CppGram v0.3!\n";

    client.run();
    return 0;
}
