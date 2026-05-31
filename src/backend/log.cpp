#include "cppgram/log.hpp"
#include "cppgram/errors.hpp"
#include <chrono>
#include <ctime>
#include <iostream>

namespace cppgram {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::set_level(LogLevel lvl) noexcept { level_ = lvl; }
LogLevel Logger::level() const noexcept { return level_; }
void Logger::enable_console(bool on) noexcept { console_ = on; }

void Logger::enable_file(const std::string& path) {
    std::lock_guard lk(mtx_);
    file_ = std::make_unique<std::ofstream>(path, std::ios::app);
    if (!file_->is_open())
        throw StorageError("Logger: cannot open log file: " + path);
}

void Logger::set_custom_sink(std::function<void(const LogRecord&)> sink) {
    std::lock_guard lk(mtx_);
    custom_ = std::move(sink);
}

void Logger::log(LogLevel lvl, std::string_view category, std::string message) {
    if (lvl < level_) return;
    emit(LogRecord{lvl, category, std::move(message)});
}

void Logger::emit(const LogRecord& rec) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t   = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char ts[32];
    std::strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tm);

    std::string line = std::format("[{}] [{:<5}] [{}] {}\n",
                                   ts, to_string(rec.level),
                                   rec.category, rec.message);

    std::lock_guard lk(mtx_);
    if (console_) {
        auto& os = (rec.level >= LogLevel::Error) ? std::cerr : std::cout;
        os << line;
    }
    if (file_ && file_->is_open()) { (*file_) << line; file_->flush(); }
    if (custom_) custom_(rec);
}

} // namespace cppgram