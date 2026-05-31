#pragma once
#include <string>
#include <string_view>
#include <functional>
#include <format>
#include <mutex>
#include <fstream>
#include <memory>

namespace cppgram {

enum class LogLevel { Trace, Debug, Info, Warn, Error, Fatal, Off };

constexpr std::string_view to_string(LogLevel l) noexcept {
    switch (l) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default:              return "OFF";
    }
}

// A sink receives an already-formatted line plus structured fields.
struct LogRecord {
    LogLevel         level;
    std::string_view category;
    std::string      message;
};

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel lvl) noexcept;
    LogLevel level() const noexcept;

    void enable_console(bool on) noexcept;
    void enable_file(const std::string& path);   // throws StorageError on failure
    void set_custom_sink(std::function<void(const LogRecord&)> sink);

    void log(LogLevel lvl, std::string_view category, std::string message);

    template <class... Args>
    void logf(LogLevel lvl, std::string_view category,
              std::format_string<Args...> fmt, Args&&... args) {
        if (lvl < level_) return;
        log(lvl, category, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    Logger() = default;
    void emit(const LogRecord& rec);

    LogLevel level_{LogLevel::Info};
    bool console_{true};
    std::unique_ptr<std::ofstream> file_;
    std::function<void(const LogRecord&)> custom_;
    std::mutex mtx_;
};

// Convenience macros — category is the compilation unit's choice.
#define CPPGRAM_LOG(lvl, cat, ...) \
    ::cppgram::Logger::instance().logf((lvl), (cat), __VA_ARGS__)

#define CPPGRAM_TRACE(cat, ...) CPPGRAM_LOG(::cppgram::LogLevel::Trace, cat, __VA_ARGS__)
#define CPPGRAM_DEBUG(cat, ...) CPPGRAM_LOG(::cppgram::LogLevel::Debug, cat, __VA_ARGS__)
#define CPPGRAM_INFO(cat,  ...) CPPGRAM_LOG(::cppgram::LogLevel::Info,  cat, __VA_ARGS__)
#define CPPGRAM_WARN(cat,  ...) CPPGRAM_LOG(::cppgram::LogLevel::Warn,  cat, __VA_ARGS__)
#define CPPGRAM_ERROR(cat, ...) CPPGRAM_LOG(::cppgram::LogLevel::Error, cat, __VA_ARGS__)
#define CPPGRAM_FATAL(cat, ...) CPPGRAM_LOG(::cppgram::LogLevel::Fatal, cat, __VA_ARGS__)

} // namespace cppgram