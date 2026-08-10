#pragma once

/**
 * @file coro.hpp
 * @brief C++20 coroutine primitives and task abstractions for asynchronous CppGram operations.
 */

#include <coroutine>
#include <exception>
#include <utility>
#include <future>
#include <optional>
#include <type_traits>
#include <mutex>
#include <condition_variable>

namespace cppgram {

template <typename T = void>
class Task;

namespace detail {

/**
 * @brief Base class for coroutine promise types supporting symmetric transfer.
 */
struct TaskPromiseBase {
    std::coroutine_handle<> continuation_{nullptr};
    std::exception_ptr exception_{nullptr};

    struct FinalAwaitable {
        bool await_ready() const noexcept { return false; }

        template <typename PromiseType>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
            auto& promise = h.promise();
            if (promise.continuation_) {
                return promise.continuation_;
            }
            return std::noop_coroutine();
        }

        void await_resume() noexcept {}
    };

    void unhandled_exception() noexcept {
        exception_ = std::current_exception();
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalAwaitable final_suspend() noexcept { return {}; }
};

/**
 * @brief Promise type for value-returning coroutine tasks.
 * @tparam T The return type of the task.
 */
template <typename T>
struct TaskPromise : public TaskPromiseBase {
    std::optional<T> value_;

    Task<T> get_return_object() noexcept;

    template <typename Value>
        requires std::convertible_to<Value&&, T>
    void return_value(Value&& val) noexcept(std::is_nothrow_constructible_v<T, Value&&>) {
        value_.emplace(std::forward<Value>(val));
    }

    T& result() {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
        return *value_;
    }
};

/**
 * @brief Promise type specialization for void-returning coroutine tasks.
 */
template <>
struct TaskPromise<void> : public TaskPromiseBase {
    Task<void> get_return_object() noexcept;

    void return_void() noexcept {}

    void result() {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
    }
};

} // namespace detail

/**
 * @brief A lazy, asynchronous C++20 coroutine task with symmetric transfer.
 * @tparam T Result type yielded by the task (default is void).
 */
template <typename T>
class Task {
public:
    using promise_type = detail::TaskPromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    Task() noexcept : handle_(nullptr) {}
    explicit Task(handle_type h) noexcept : handle_(h) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    ~Task() {
        if (handle_) handle_.destroy();
    }

    /**
     * @brief Checks if the task has finished execution.
     */
    bool is_ready() const noexcept {
        return !handle_ || handle_.done();
    }

    auto operator co_await() const& noexcept = delete;

    auto operator co_await() && noexcept {
        struct Awaitable {
            handle_type handle_;

            bool await_ready() const noexcept {
                return !handle_ || handle_.done();
            }

            std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting_coro) noexcept {
                handle_.promise().continuation_ = awaiting_coro;
                return handle_;
            }

            decltype(auto) await_resume() {
                if (!handle_) {
                    throw std::runtime_error("Attempted to await empty Task");
                }
                return handle_.promise().result();
            }
        };

        return Awaitable{handle_};
    }

    handle_type handle() const noexcept { return handle_; }

private:
    handle_type handle_;
};

namespace detail {

template <typename T>
inline Task<T> TaskPromise<T>::get_return_object() noexcept {
    return Task<T>{std::coroutine_handle<TaskPromise<T>>::from_promise(*this)};
}

inline Task<void> TaskPromise<void>::get_return_object() noexcept {
    return Task<void>{std::coroutine_handle<TaskPromise<void>>::from_promise(*this)};
}

} // namespace detail

/**
 * @brief Synchronously blocks the calling thread until a value-returning Task completes.
 * @tparam T The return type of the task.
 * @param task The coroutine task to execute.
 * @return The result returned by the completed coroutine.
 */
template <typename T>
inline T sync_wait(Task<T> task) {
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    std::optional<T> res;
    std::exception_ptr exc;

    auto runner = [&]() -> Task<void> {
        try {
            if constexpr (std::is_void_v<T>) {
                co_await std::move(task);
            } else {
                res.emplace(co_await std::move(task));
            }
        } catch (...) {
            exc = std::current_exception();
        }
        {
            std::lock_guard<std::mutex> lk(mtx);
            done = true;
        }
        cv.notify_one();
    };

    auto run_task = runner();
    run_task.handle().resume();

    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [&] { return done; });

    if (exc) {
        std::rethrow_exception(exc);
    }
    if constexpr (!std::is_void_v<T>) {
        return std::move(*res);
    }
}

/**
 * @brief Synchronously blocks the calling thread until a void-returning Task completes.
 * @param task The void coroutine task to execute.
 */
inline void sync_wait(Task<void> task) {
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    std::exception_ptr exc;

    auto runner = [&]() -> Task<void> {
        try {
            co_await std::move(task);
        } catch (...) {
            exc = std::current_exception();
        }
        {
            std::lock_guard<std::mutex> lk(mtx);
            done = true;
        }
        cv.notify_one();
    };

    auto run_task = runner();
    run_task.handle().resume();

    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [&] { return done; });

    if (exc) {
        std::rethrow_exception(exc);
    }
}

} // namespace cppgram
