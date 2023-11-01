#include "thread_pool.hpp"

ThreadPool::ThreadPool() noexcept
{
    uint32_t available = std::thread::hardware_concurrency();
    uint32_t numThreads = available > 8 ? 8 : available;

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

ThreadPool::~ThreadPool() noexcept
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