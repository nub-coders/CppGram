// examples/hello.cpp — proves the Phase 1 build links and runs.
#include "cppgram/client.hpp"
#include "cppgram/log.hpp"
#include <iostream>

int main() {
    using namespace cppgram;
    Logger::instance().set_level(LogLevel::Trace);

    Client client(12345, "0123456789abcdef0123456789abcdef");
    auto msg = client.sendMessage(42, "Hello World");
    std::cout << "Queued message to chat " << msg.chat_id
              << ": " << msg.text << "\n";
    return 0;
}