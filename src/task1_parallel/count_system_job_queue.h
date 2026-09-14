// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef COUNT_SYSTEM_JOB_QUEUE_H
#define COUNT_SYSTEM_JOB_QUEUE_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#include "count_system_job.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct CountSystemJobQueue
{
    //---------------------------------------------------------------------------------------//
    // Internal data.
    //---------------------------------------------------------------------------------------//
    bool m_shutdown = false;
    std::queue<CountSystemJob> m_queue;

    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Constructor and Destructor.
    //---------------------------------------------------------------------------------------//
    CountSystemJobQueue() = default;
    ~CountSystemJobQueue() = default;

    inline void shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = true;
        }

        m_condition_variable.notify_all();
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Insert and Retrieve jobs.
    //---------------------------------------------------------------------------------------//
    inline bool insertCountSystemJob(const CountSystemJob& count_system_job)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(m_shutdown) { return false; }

            m_queue.push(count_system_job);
        }
        
        m_condition_variable.notify_one();
        return true;
    }

    inline bool getCountSystemJob(CountSystemJob& output)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_condition_variable.wait
        (
            lock,
            [this]()
            {
                return m_shutdown || !m_queue.empty();
            }
        );

        if(m_shutdown && m_queue.empty()) { return false; }

        output = std::move(m_queue.front());
        m_queue.pop();

        return true;
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //