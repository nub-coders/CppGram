#pragma once

/**
 * @file thread_pool.hpp
 * @brief Thread-safe worker pool for concurrent event dispatching and background tasks.
 */

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

namespace cppgram {

/**
 * @brief Manages a fixed-size pool of worker threads executing enqueued tasks concurrently.
 */
class ThreadPool {
public:
    /**
     * @brief Constructs a ThreadPool with the specified number of worker threads.
     * @param threads Number of worker threads (default is hardware concurrency).
     */
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency())
        : stop_(false), active_tasks_(0), completed_tasks_(0) {
        if (threads == 0) threads = 1;
        workers_.reserve(threads);
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);
                        this->cv_.wait(lock, [this] {
                            return this->stop_ || !this->tasks_.empty();
                        });
                        if (this->stop_ && this->tasks_.empty()) {
                            return;
                        }
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                        ++active_tasks_;
                    }
                    try {
                        task();
                    } catch (...) {
                        // Swallow exceptions to keep worker threads alive
                    }
                    --active_tasks_;
                    ++completed_tasks_;
                    completion_cv_.notify_all();
                }
            });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        shutdown();
    }

    /**
     * @brief Enqueues a callable task for execution in the thread pool.
     * @tparam F Callable type.
     * @tparam Args Argument types.
     * @return Future holding the eventual result of the task.
     */
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [f = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                return f(args...);
            }
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("ThreadPool::enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return res;
    }

    /**
     * @brief Returns the number of worker threads.
     */
    size_t size() const noexcept {
        return workers_.size();
    }

    /**
     * @brief Returns the number of tasks currently queued and waiting.
     */
    size_t queued_tasks() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    /**
     * @brief Returns the number of tasks currently actively executing.
     */
    size_t active_tasks() const noexcept {
        return active_tasks_.load();
    }

    /**
     * @brief Returns the cumulative number of tasks executed by the thread pool.
     */
    size_t completed_tasks() const noexcept {
        return completed_tasks_.load();
    }

    /**
     * @brief Blocks the caller until all queued and active tasks have finished.
     */
    void wait_all() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        completion_cv_.wait(lock, [this] {
            return tasks_.empty() && active_tasks_ == 0;
        });
    }

    /**
     * @brief Checks if the thread pool is currently accepting and running tasks.
     */
    bool is_running() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return !stop_;
    }

    /**
     * @brief Signals workers to stop after finishing remaining queue tasks and joins threads.
     */
    void shutdown() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) return;
            stop_ = true;
        }
        cv_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::condition_variable completion_cv_;
    bool stop_;
    std::atomic<size_t> active_tasks_;
    std::atomic<size_t> completed_tasks_;
};

} // namespace cppgram
