// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef COUNT_SYSTEM_H
#define COUNT_SYSTEM_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../../bpe.h"
#include "count_system_job.h"
#include "count_system_job_queue.h"
#include "count_system_total_jobs_counter.h"
#include "count_system_worker.h"
#include "word_counts.h"

// Standard library.
#include <memory>
#include <vector>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct CountSystem
{
    //---------------------------------------------------------------------------------------//
    // External data. Must exist for lifetime of CountSystem object.
    //---------------------------------------------------------------------------------------//
    const std::vector<bpe::Word>& m_words;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Internal data.
    //---------------------------------------------------------------------------------------//
    std::size_t                                     m_chunk_size        ;
    WordCounts                                      m_word_counts       ;
    CountSystemJobQueue                             m_job_queue         ;
    CountSystemTotalJobsCounter                     m_total_jobs_counter;
    std::vector<std::unique_ptr<CountSystemWorker>> m_workers           ;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Constructor and Destructor.
    //---------------------------------------------------------------------------------------//
    CountSystem(const std::vector<bpe::Word>& words, std::size_t chunk_size, std::size_t num_workers)
    :   m_words(words)
    ,   m_chunk_size(chunk_size)
    ,   m_word_counts(words)
    ,   m_total_jobs_counter(0)
    {
        m_workers.reserve(num_workers);
        for(std::size_t i = 0; i < num_workers; i++)
        {
            m_workers.emplace_back
            (
                std::make_unique<CountSystemWorker>
                (
                    m_words,
                    m_word_counts,
                    m_job_queue,
                    m_total_jobs_counter
                )
            );
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    inline void countWords()
    {
        std::size_t index = 0;
        while(index < m_words.size())
        {
            std::size_t start_index = index;
            std::size_t end_index   = index + m_chunk_size - 1;

            if(end_index >= m_words.size())
            {
                end_index = m_words.size() - 1;
            }

            m_total_jobs_counter.increment();
            bool check_inserted = m_job_queue.insertCountSystemJob(CountSystemJob(start_index, end_index));
            if(check_inserted == false)
            {
                m_total_jobs_counter.decrement();
            }
            m_total_jobs_counter.waitUntilZero();

            index += m_chunk_size;
        }
        m_total_jobs_counter.waitUntilZero();
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //