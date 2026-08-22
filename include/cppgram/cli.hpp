#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <iostream>
#include <sstream>

namespace cppgram {

struct CommandContext {
    std::string command_name;
    std::vector<std::string> args;
    std::ostream& out;
};

using CommandHandler = std::function<void(const CommandContext&)>;

struct CommandInfo {
    std::string name;
    std::string description;
    std::string usage;
    CommandHandler handler;
};

class InteractiveCLI {
public:
    InteractiveCLI();

    void register_command(
        const std::string& name,
        const std::string& description,
        const std::string& usage,
        CommandHandler handler);

    void set_prompt(const std::string& prompt) { prompt_ = prompt; }
    void set_banner(const std::string& banner) { banner_ = banner; }

    bool execute_line(const std::string& line, std::ostream& out = std::cout);
    void run(std::istream& in = std::cin, std::ostream& out = std::cout);

    [[nodiscard]] bool has_command(const std::string& name) const;
    [[nodiscard]] const std::map<std::string, CommandInfo>& get_commands() const noexcept { return commands_; }

private:
    void register_builtin_commands();
    static std::vector<std::string> parse_args(const std::string& line);

    std::map<std::string, CommandInfo> commands_;
    std::string prompt_{"cppgram> "};
    std::string banner_;
    bool running_{true};
};

} // namespace cppgram
