// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef COUNT_SYSTEM_WORKER_H
#define COUNT_SYSTEM_WORKER_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../../bpe.h"
#include "count_system_job.h"
#include "count_system_job_queue.h"
#include "count_system_total_jobs_counter.h"
#include "word_counts.h"

// Standard library.
#include <thread>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct CountSystemWorker
{
    //---------------------------------------------------------------------------------------//
    // External data. Must exist for lifetime of CountSystemWorker object.
    //---------------------------------------------------------------------------------------//
    const std::vector<bpe::Word>& m_words             ;
    WordCounts&                   m_word_counts       ;
    CountSystemJobQueue&          m_job_queue         ;
    CountSystemTotalJobsCounter&  m_total_jobs_counter;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Internal data.
    //---------------------------------------------------------------------------------------//
    std::thread m_worker_thread;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Constructor and Destructor.
    //---------------------------------------------------------------------------------------//
    CountSystemWorker
    (
        const std::vector<bpe::Word>& words             ,
        WordCounts&                   word_counts       ,
        CountSystemJobQueue&          job_queue         ,
        CountSystemTotalJobsCounter&  total_jobs_counter
    )
    :   m_words(words)
    ,   m_word_counts(word_counts)
    ,   m_job_queue(job_queue)
    ,   m_total_jobs_counter(total_jobs_counter)
    {
        this->start();
    }

    ~CountSystemWorker()
    {
        this->stop();
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Start and stop worker thread.
    //---------------------------------------------------------------------------------------//
    inline void start()
    {
        if(m_worker_thread.joinable()) { return; }

        m_worker_thread = std::thread(&CountSystemWorker::workerFunction, this);
    }

    inline void stop()
    {
        m_job_queue.shutdown();

        if(m_worker_thread.joinable()) { m_worker_thread.join(); }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Worker function.
    //---------------------------------------------------------------------------------------//
    inline void workerFunction()
    {
        CountSystemJob count_system_job;

        while(true)
        {
            if(m_job_queue.getCountSystemJob(count_system_job) == false) { break; }

            std::size_t start_index = count_system_job.m_start;
            std::size_t end_index   = count_system_job.m_end;

            for(std::size_t i = start_index; i <= end_index; i++)
            {
                if(i >= m_words.size())
                {
                    continue;
                }
                m_word_counts.incrementWordCount(m_words[i]);
            }

            m_total_jobs_counter.decrement();
        }
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //