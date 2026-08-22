#include "cppgram/cli.hpp"
#include <iomanip>

namespace cppgram {

InteractiveCLI::InteractiveCLI() {
    register_builtin_commands();
}

void InteractiveCLI::register_command(
    const std::string& name,
    const std::string& description,
    const std::string& usage,
    CommandHandler handler) {
    commands_[name] = {name, description, usage, std::move(handler)};
}

bool InteractiveCLI::has_command(const std::string& name) const {
    return commands_.find(name) != commands_.end();
}

void InteractiveCLI::register_builtin_commands() {
    register_command(
        "/help",
        "Display available commands and descriptions",
        "/help [command]",
        [this](const CommandContext& ctx) {
            if (ctx.args.empty()) {
                ctx.out << "=== Available Commands ===\n";
                for (const auto& [name, info] : commands_) {
                    ctx.out << "  " << std::left << std::setw(15) << name
                            << " : " << info.description << "\n";
                }
                ctx.out << "Type '/help <cmd>' for command usage details.\n";
            } else {
                auto it = commands_.find(ctx.args[0]);
                if (it != commands_.end()) {
                    ctx.out << "Command: " << it->second.name << "\n"
                            << "Description: " << it->second.description << "\n"
                            << "Usage: " << it->second.usage << "\n";
                } else {
                    ctx.out << "Unknown command: " << ctx.args[0] << "\n";
                }
            }
        });

    register_command(
        "/clear",
        "Clear the console screen",
        "/clear",
        [](const CommandContext& ctx) {
            ctx.out << "\033[2J\033[1;1H";
        });

    register_command(
        "/exit",
        "Exit the interactive shell",
        "/exit",
        [this](const CommandContext& ctx) {
            ctx.out << "Exiting CppGram CLI. Goodbye!\n";
            running_ = false;
        });

    register_command(
        "/quit",
        "Exit the interactive shell",
        "/quit",
        [this](const CommandContext& ctx) {
            ctx.out << "Exiting CppGram CLI. Goodbye!\n";
            running_ = false;
        });
}

std::vector<std::string> InteractiveCLI::parse_args(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if ((c == ' ' || c == '\t') && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

bool InteractiveCLI::execute_line(const std::string& line, std::ostream& out) {
    auto tokens = parse_args(line);
    if (tokens.empty()) {
        return running_;
    }

    std::string cmd = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    auto it = commands_.find(cmd);
    if (it != commands_.end()) {
        CommandContext ctx{cmd, args, out};
        it->second.handler(ctx);
    } else {
        out << "Unknown command '" << cmd << "'. Type '/help' for a list of commands.\n";
    }

    return running_;
}

void InteractiveCLI::run(std::istream& in, std::ostream& out) {
    if (!banner_.empty()) {
        out << banner_ << "\n";
    }

    running_ = true;
    std::string line;
    while (running_) {
        out << prompt_;
        out.flush();
        if (!std::getline(in, line)) {
            break;
        }
        if (!execute_line(line, out)) {
            break;
        }
    }
}

} // namespace cppgram
