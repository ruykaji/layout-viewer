#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
    std::vector<std::thread> m_workers {};
    std::queue<std::function<void()>> m_tasks {};

    std::mutex m_queueMutex {};
    std::mutex m_completionMutex {};

    std::condition_variable m_condition {};
    std::condition_variable m_completionCondition;

    std::atomic<std::size_t> m_tasksEnqueued;
    std::atomic<std::size_t> m_tasksCompleted;

    std::atomic<bool> m_stop { false };

public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool() noexcept;
    ~ThreadPool() noexcept;

    template <class F, class... Args>
    inline void enqueue(F&& t_function, Args&&... t_args) noexcept
    {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);

            m_tasks.emplace(std::bind(std::forward<F>(t_function), std::forward<Args>(t_args)...));
            ++m_tasksEnqueued;
        }

        m_condition.notify_one();
    }

    inline void wait() noexcept
    {
        std::unique_lock<std::mutex> completionLock(m_completionMutex);

        m_completionCondition.wait(completionLock, [this] {
            return m_tasksCompleted == m_tasksEnqueued;
        });
    };
};

#endif