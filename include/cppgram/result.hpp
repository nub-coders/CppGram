#pragma once
#include <variant>
#include <optional>
#include <utility>
#include "cppgram/errors.hpp"

namespace cppgram {

// Minimal std::expected-style type for internal hot paths (pre-C++23).
template <class T>
class Result {
public:
    Result(T value) : data_(std::move(value)) {}
    Result(Error err) : data_(std::move(err)) {}

    bool ok()        const noexcept { return std::holds_alternative<T>(data_); }
    explicit operator bool() const noexcept { return ok(); }

    T&        value()       { return std::get<T>(data_); }
    const T&  value() const { return std::get<T>(data_); }
    const Error& error() const { return std::get<Error>(data_); }

    // Throws the mapped exception if this holds an error (API boundary helper).
    T& unwrap() {
        if (!ok()) error().raise();
        return std::get<T>(data_);
    }

private:
    std::variant<T, Error> data_;
};

template <>
class Result<void> {
public:
    Result() : err_(std::nullopt) {}
    Result(Error err) : err_(std::move(err)) {}
    bool ok() const noexcept { return !err_.has_value(); }
    explicit operator bool() const noexcept { return ok(); }
    const Error& error() const { return *err_; }
    void unwrap() { if (err_) err_->raise(); }
private:
    std::optional<Error> err_;
};

} // namespace cppgram