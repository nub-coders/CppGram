// tests/phase1_smoke.cpp
#include "cppgram/client.hpp"
#include "cppgram/errors.hpp"
#include <cassert>

int main() {
    cppgram::Client c(1, "hash");
    auto m = c.sendMessage(7, "ping");
    assert(m.chat_id == 7 && m.text == "ping" && m.outgoing);

    bool threw = false;
    try { c.login("+10000000000"); }
    catch (const cppgram::AuthenticationError&) { threw = true; }
    assert(threw);

    return 0;
}