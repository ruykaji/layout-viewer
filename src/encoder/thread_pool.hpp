#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <atomic>
#include <iostream>
#include <condition_variable>
#include <cstdint>
#include <functional>
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

    ThreadPool()
    {
        uint32_t available = std::thread::hardware_concurrency();
        uint32_t numThreads = available > 4 ? 4 : available;

        m_workers.reserve(numThreads);

        for (std::size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->m_queueMutex);

                        this->m_condition.wait(lock, [this] {
                            return this->m_stop || !this->m_tasks.empty();
                        });

                        if (this->m_stop && this->m_tasks.empty()) {
                            return;
                        }

                        task = std::move(this->m_tasks.front());
                        this->m_tasks.pop();
                    }

                    task();

                    {
                        std::unique_lock<std::mutex> completionLock(m_completionMutex);
                        ++m_tasksCompleted;
                        m_completionCondition.notify_all();
                    }
                }
            });
        }
    }

    ~ThreadPool()
    {
        wait();

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_stop = true;
        }

        m_condition.notify_all();

        for (auto& worker : m_workers) {
            worker.join();
        }
    }

    template <class F, class... Args>
    void enqueue(F&& t_function, Args&&... t_args)
    {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            
            m_tasks.emplace(std::bind(std::forward<F>(t_function), std::forward<Args>(t_args)...));
            ++m_tasksEnqueued;
        }

        m_condition.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> completionLock(m_completionMutex);
    
        m_completionCondition.wait(completionLock, [this] {
            return m_tasksCompleted == m_tasksEnqueued;
        });
    }
};

#endif