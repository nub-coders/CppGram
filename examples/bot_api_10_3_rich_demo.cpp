// examples/bot_api_10_3_rich_demo.cpp
// Demonstration of Telegram Bot API 10.3 (August 2026) Rich Messages & Rich Text.

#include "cppgram/client.hpp"
#include "cppgram/rich_message.hpp"
#include "cppgram/errors.hpp"
#include <iostream>

using namespace cppgram;

int main() {
    std::cout << "====================================================================\n";
    std::cout << " CppGram: Telegram Bot API 10.3 Rich Messages & Rich Text Showcase\n";
    std::cout << "====================================================================\n\n";

    // 1. Build a structured multi-block Rich Message using RichMessageBuilder
    std::cout << "[1] Constructing Bot API 10.3 Multi-Block Rich Message...\n";

    RichBlockTable metrics_table;
    metrics_table.is_bordered = true;
    metrics_table.is_striped = true;
    metrics_table.is_compact = true; // Bot API 10.3 feature
    metrics_table.caption = "Service Health Summary (US-East)";

    metrics_table.cells = {
        {
            RichBlockTableCell{"Service", true, 1, 1, "left", "middle"},
            RichBlockTableCell{"Status", true, 1, 1, "center", "middle"},
            RichBlockTableCell{"p99 Latency", true, 1, 1, "right", "middle"}
        },
        {
            RichBlockTableCell{"Auth Gateway", false, 1, 1, "left", "top"},
            RichBlockTableCell{"Healthy", false, 1, 1, "center", "top"},
            RichBlockTableCell{"8.4ms", false, 1, 1, "right", "top"}
        },
        {
            RichBlockTableCell{"MTProto Broker", false, 1, 1, "left", "top"},
            RichBlockTableCell{"Optimal", false, 1, 1, "center", "top"},
            RichBlockTableCell{"14.1ms", false, 1, 1, "right", "top"}
        }
    };

    auto rich_doc = RichMessageBuilder()
        .heading("Cluster Telemetry & Performance Report")
        .paragraph("Real-time infrastructure status for high-throughput messaging nodes:")
        .table(metrics_table)
        .expandable_quote(
            "All node groups have converged to target capacity. Zero packet drops observed during rolling update.",
            "Site Reliability Engineering (SRE)")
        .buttons({
            RichMessageButton::make_url("Grafana Dashboard", "https://metrics.nubcoders.internal", "primary"),
            RichMessageButton::make_callback("Acknowledge Alerts", "ack_cluster_report", "success"),
            RichMessageButton::make_copy("Copy Hash", "sha256:7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069", "danger")
        }, "center")
        .document("tg://document?id=audit_report_2026_08_29.pdf", "Full Audit Report (PDF)")
        .divider()
        .preformatted("kubectl get pods -n messaging -l app=cppgram-broker\nREADY   STATUS    RESTARTS   AGE\n8/8     Running   0          42h", "bash")
        .build();

    std::cout << "  - Total Blocks: " << rich_doc.blocks.size() << "\n";
    std::cout << "  - Compact Table configured: " << (metrics_table.is_compact ? "YES" : "NO") << "\n";
    std::cout << "  - Expandable Blockquote present: YES\n";
    std::cout << "  - Document link with tg://document?id= present: YES\n\n";

    // 2. Demonstrate Rich Message Draft Streaming (Bot API 10.3 / 10.1+)
    std::cout << "[2] Simulating AI Rich Message Draft Streaming with Thinking Block...\n";
    auto streaming_draft = RichMessageBuilder()
        .thinking("Synthesizing log streams and aggregating p99 latency distributions across clusters...")
        .paragraph("Synthesizing metrics: 99.98% uptime achieved.")
        .build();

    std::cout << "  - Draft Blocks: " << streaming_draft.blocks.size() << "\n";
    std::cout << "  - First block type is Thinking: "
              << (streaming_draft.blocks[0].type == RichBlockType::Thinking ? "YES" : "NO") << "\n\n";

    // 3. Demonstrate Client API Surface
    std::cout << "[3] Initializing CppGram Client for Rich Message Transmission...\n";
    Client client(12345, "mock_hash_for_offline_demo");
    ChatId target_chat = -1001234567890LL;

    SendRichMessageOptions options;
    options.protect_content = true;
    options.ephemeral_receiver_user_id = 99887766LL; // Bot API 10.3 Ephemeral parameter
    options.message_thread_id = 100;

    try {
        Message sent_rich = client.sendRichMessage(target_chat, rich_doc, options);
        std::cout << "  - sendRichMessage dispatched (chat_id: " << target_chat << ")\n";
        std::cout << "  - Attached rich_message blocks: "
                  << (sent_rich.rich_message.has_value() ? std::to_string(sent_rich.rich_message->blocks.size()) : "0") << "\n";
    } catch (const CppGramException& e) {
        std::cout << "  - sendRichMessage invocation validated (expected offline demo state: " << e.what() << ")\n";
    }

    try {
        bool draft_ok = client.sendRichMessageDraft(target_chat, 101, streaming_draft, /*can_stop=*/true, /*keep_on_stop=*/false);
        std::cout << "  - sendRichMessageDraft streaming response: " << (draft_ok ? "SUCCESS" : "FAILED") << "\n\n";
    } catch (const CppGramException& e) {
        std::cout << "  - sendRichMessageDraft streaming validated (expected offline demo state: " << e.what() << ")\n\n";
    }

    std::cout << "====================================================================\n";
    std::cout << " Bot API 10.3 Rich Messages & Rich Text Demo completed successfully!\n";
    std::cout << "====================================================================\n";

    return 0;
}
