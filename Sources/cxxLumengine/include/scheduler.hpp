#ifndef LE_SCHEDULER_HPP
#define LE_SCHEDULER_HPP

#include "thread_pool.hpp"

class LeScheduler final {
    ThreadPoolPtr m_pool;

public:
    explicit LeScheduler(const std::size_t thread_count): m_pool(
        ThreadPool::create_thread_pool(thread_count)) {
    }
    
    explicit LeScheduler(): m_pool(
       ThreadPool::create_thread_pool(std::thread::hardware_concurrency())) {
   }

    // Delete copy operations
    LeScheduler(const LeScheduler &) = delete;

    LeScheduler &operator=(const LeScheduler &) = delete;

    // Default move operations
    LeScheduler(LeScheduler &&) noexcept = default;

    LeScheduler &operator=(LeScheduler &&) noexcept = default;

    void run_immediately(Workload workload) const {
        m_pool->run_immediately(std::move(workload));
    }

    void run_at(Workload workload, const PointInTime time) const {
        m_pool->run_at(std::move(workload), time);
    }

    void run_after(Workload workload, const std::chrono::nanoseconds delay) const {
        m_pool->run_after(std::move(workload), delay);
    }
};

#endif //LE_SCHEDULER_HPP
