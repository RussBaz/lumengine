#ifndef LE_THREAD_POOL_HPP
#define LE_THREAD_POOL_HPP

#include <cxxAsio.hpp>
#include <vector>
#include <thread>

#include "sparse_vector.hpp"
#include "workload.hpp"

class ScheduledWorkload final {
    asio::io_context& m_io_context;
    asio::strand<asio::any_io_executor> m_strand;
    Workload m_workload;
    PointInTime m_scheduled_at_time { std::chrono::steady_clock::now() };
    std::optional<asio::steady_timer> m_timer { std::nullopt };
    std::function<void()> m_cleanup_action;
    SparseVector<Server>& m_server_storage;
    bool m_started { false };
    bool m_finished { false };

    void run_workload(const std::error_code& error) {
        auto do_immediate_cleanup = true;
        if (!error) {
            m_started = true;
            m_workload.workload.visit_all_cases(
                [] (const FunctionWorkload& wl) {
                    wl.call();
                },
                [this, &do_immediate_cleanup] (const StartServerWorkload& wl) {
                    if (!m_server_storage.contains([&wl](const Server& s) {
                        return s.port() == wl.config->port();
                    })) {
                        m_server_storage.add(Server(m_io_context, m_strand, wl.config, [this]{
                            m_finished = true;
                            m_cleanup_action();
                        }));
                        do_immediate_cleanup = false;
                    }
                },
                [this] (const StopServerWorkload& wl) {
                    const auto server = m_server_storage.first_where([&wl](const Server& s) {
                        return s.port() == wl.port;
                    });

                    if (server) {
                        server->stop();
                    }
                }
            );
        }

        if (m_workload.callback) {
            if (error) {
                m_workload.callback->call(error);
            } else {
                m_workload.callback->call(make_error_code(CustomErrorCode::Success));
            }
        }

        if (do_immediate_cleanup) {
            m_finished = true;
            m_cleanup_action();
        }
    }
public:
    ScheduledWorkload(
        asio::io_context& io,
        Workload workload,
        const VariantWrapper<ExecuteSchedule> schedule,
        SparseVector<Server>& server_storage,
        std::function<void()> cleanup_action = []{}
    ) : m_io_context(io),
        m_strand { make_strand(io) },
        m_workload { std::move(workload) },
        m_cleanup_action { std::move(cleanup_action) },
        m_server_storage { server_storage } {
        schedule.visit_all_cases(
            [this](ExecuteNow) {
                // Immediately execute the workload
                post(
                    m_strand,
                    [this] {
                        run_workload(make_error_code(CustomErrorCode::Success));
                    }
                );
            },
            [this, &io](const ExecuteAt at) {
                // Schedule the workload to be executed at a specific time
                m_timer.emplace(asio::steady_timer(io, at.start_time));
                m_timer->async_wait(bind_executor(
                    m_strand,
                    [this](const std::error_code& error) {
                        run_workload(error);
                    }
                ));
            },
            [this, &io](const ExecuteAfter after) {
                // Schedule the workload to be executed after a specific delay
                m_timer.emplace(io, std::chrono::steady_clock::now() + after.delay);
                m_timer->async_wait(bind_executor(
                    m_strand,
                    [this](const std::error_code& error) {
                        run_workload(error);
                    }
                ));
            }
        );
    }

    // Cancel the scheduled workload. This method is thread safe as it uses a strand
    // to ensure that the timer is accessed in a thread-safe manner.
    void cancel() {
        post(m_strand, [this] {
            if (m_timer) {
                m_timer->cancel();
            }
        });
    }

    [[nodiscard]] bool started() const {
        std::promise<bool> promise;
        auto future = promise.get_future();
        post(m_strand, [this, &promise] {
            promise.set_value(m_started);
        });
        return future.get();
    }

    [[nodiscard]] bool finished() const {
        std::promise<bool> promise;
        auto future = promise.get_future();
        post(m_strand, [this, &promise] {
            promise.set_value(m_finished);
        });
        return future.get();
    }

    [[nodiscard]] PointInTime scheduled_at_time() const {
        return m_scheduled_at_time;
    }
};

class ThreadPool final {
    std::size_t m_num_threads;
    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    asio::strand<asio::any_io_executor> m_cleanup_strand;
    std::vector<std::thread> m_threads;
    SparseVector<ScheduledWorkload> m_workloads;
    SparseVector<Server> m_running_servers;

    void schedule_workload(Workload workload, const VariantWrapper<ExecuteSchedule> schedule) {
        m_workloads.add(ScheduledWorkload {
            m_io_context,
            std::move(workload),
            schedule,
            m_running_servers,
            [this] {
                post(m_cleanup_strand, [this] {
                    remove_completed_workloads();
                });
            }
        });
    }

    void remove_completed_workloads() {
        auto it = m_workloads.begin();
        while (it != m_workloads.end()) {
            if (it->finished()) {
                const size_t pos = it.position();
                ++it;
                m_workloads.remove(pos);
            } else {
                ++it;
            }
        }
    }
public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    explicit ThreadPool(const std::size_t num_threads)
        : m_num_threads { std::max(std::size_t { 1 }, num_threads) },
          m_work_guard { make_work_guard(m_io_context) },
          m_cleanup_strand { make_strand(m_io_context) },
          m_workloads { m_num_threads*32 },
          m_running_servers { m_num_threads } {
        for (std::size_t i = 0; i < m_num_threads; ++i) {
            m_threads.emplace_back([this] { m_io_context.run(); });
        }
    }

    ~ThreadPool() {
        m_work_guard.reset();
        m_io_context.stop();
        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void run_immediately(Workload workload) {
        schedule_workload(std::move(workload), VariantWrapper<ExecuteSchedule> { ExecuteNow{} } );
    }

    void run_at(Workload workload, const PointInTime time) {
        schedule_workload(std::move(workload), VariantWrapper<ExecuteSchedule> { ExecuteAt { time } });
    }

    void run_after(Workload workload, const std::chrono::nanoseconds delay) {
        schedule_workload(std::move(workload), VariantWrapper<ExecuteSchedule> { ExecuteAfter { delay }});
    }

    static std::shared_ptr<ThreadPool> create_thread_pool(std::size_t num_threads) {
        return std::make_shared<ThreadPool>(num_threads);
    }

    // Check if there are any active workloads or servers
    [[nodiscard]] bool has_active_tasks() const {
        return !m_workloads.empty() || !m_running_servers.empty();
    }

    // // Wait for all current workloads to complete
    // void wait_for_completion() {
    //     std::promise<void> completion_promise;
    //     const auto completion_future = completion_promise.get_future();

    //     // Create a timer that we'll reuse
    //     auto timer = std::make_shared<asio::steady_timer>(m_io_context);

    //     // Create the check function
    //     std::function<void()> check_completion;
    //     check_completion = [this, timer, &check_completion, completion_promise = std::move(completion_promise)]() mutable {
    //         post(m_cleanup_strand, [this, timer, &check_completion, completion_promise = std::move(completion_promise)]() mutable {
    //             remove_completed_workloads();
    //             if (!has_active_tasks()) {
    //                 completion_promise.set_value();
    //             } else {
    //                 timer->expires_after(std::chrono::milliseconds(100));
    //                 timer->async_wait([this, timer, &check_completion, completion_promise = std::move(completion_promise)]
    //                     (const std::error_code&) mutable {
    //                         check_completion();
    //                 });
    //             }
    //         });
    //     };

    //     // Start the first check
    //     check_completion();

    //     // Wait for completion
    //     completion_future.wait();
    // }
};

using ThreadPoolPtr = std::shared_ptr<ThreadPool>;

#endif // LE_THREAD_POOL_HPP
