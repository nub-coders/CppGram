#include "cppgram/cli.hpp"
#include "cppgram/network.hpp"
#include "cppgram/session.hpp"
#include <iostream>

using namespace cppgram;

int main() {
    InteractiveCLI cli;
    DatacenterManager dc_mgr;
    Session current_session;

    cli.set_banner(
        "=======================================================\n"
        "             CppGram Interactive Bot & MTProto Shell  \n"
        "=======================================================\n"
        "Type /help for a list of available commands.\n"
        "Type /exit or /quit to terminate the session."
    );

    cli.register_command(
        "/dc",
        "View or select active Telegram DataCenter",
        "/dc [id]",
        [&dc_mgr](const CommandContext& ctx) {
            if (ctx.args.empty()) {
                auto primary = dc_mgr.get_primary_dc();
                ctx.out << "Primary DataCenter: DC " << primary->id << " (" << primary->name << ") at "
                        << primary->ip_v4 << ":" << primary->port << "\n";
            } else {
                int id = std::stoi(ctx.args[0]);
                auto dc = dc_mgr.get_dc(id);
                if (dc) {
                    dc_mgr.set_primary_dc(id);
                    ctx.out << "Switched primary DataCenter to DC " << id << " (" << dc->name << ")\n";
                } else {
                    ctx.out << "Invalid DC id. Available: 1, 2, 3, 4, 5\n";
                }
            }
        });

    cli.register_command(
        "/session",
        "Display current session details and message statistics",
        "/session",
        [&current_session](const CommandContext& ctx) {
            ctx.out << "Session ID: 0x" << std::hex << current_session.get_session_id() << std::dec << "\n";
            ctx.out << "Server Salt: 0x" << std::hex << current_session.get_server_salt() << std::dec << "\n";
            ctx.out << "Next Msg ID: " << current_session.generate_msg_id() << "\n";
        });

    cli.register_command(
        "/status",
        "Check system status and network connectivity",
        "/status",
        [](const CommandContext& ctx) {
            ctx.out << "Engine Status: Ready (C++20 MTProto 2.0 Engine)\n";
            ctx.out << "Active Threads: Pool Idle\n";
        });

    cli.run();
    return 0;
}
