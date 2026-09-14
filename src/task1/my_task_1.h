// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef MY_TASK_1_H
#define MY_TASK_1_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../bpe.h"

#include "absl/log/log.h"
#include "count_system/count_system.h"
#include "merge_sort_system/merge_sort_system.h"
// ##### ###### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct MyTask1
{
private:
    //---------------------------------------------------------------------------------------//
    /*
    -   Following code copied from task1.cpp:
        -   Function: elapsed_ms()
    */
    //---------------------------------------------------------------------------------------//
    std::int64_t elapsed_ms
    (
        const std::chrono::steady_clock::time_point& start,
        const std::chrono::steady_clock::time_point& end
    )
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    //---------------------------------------------------------------------------------------//

public:
    inline void my_task1(const std::vector<bpe::Word>& words, bpe::Results& results)
    {
        results.word_counts.clear();
        results.char_splits.clear();

        size_t num_count_system_workers = 16;
        CountSystem count_system(words, words.size() / num_count_system_workers, num_count_system_workers);

        MergeSortSystem merge_sort_system;

        const std::chrono::steady_clock::time_point t_wc0 = std::chrono::steady_clock::now();

        if(words.empty())
        {
            LOG(INFO) << "word count: 0 ms; char split: 0 ms";
            return;
        }

        //-----------------------------------------------------------------------------------//
        // Task 1.1.1: Word frequency counting.
        //-----------------------------------------------------------------------------------//
        count_system.countWords();
        //count_system.m_word_counts.printWordCounts(std::string("out_1_1_1_word_counts.txt"));
        //-----------------------------------------------------------------------------------//

        //-----------------------------------------------------------------------------------//
        // Task 1.1.2: Sort Words.
        //-----------------------------------------------------------------------------------//
        std::vector<std::pair<const bpe::Byte*, std::size_t>> sorted;
        sorted.reserve(count_system.m_word_counts.m_word_counts.size());
        for (const auto& entry : count_system.m_word_counts.m_word_counts)
        {
            sorted.emplace_back(entry.first, entry.second.getCount());
        }
        merge_sort_system.parallelMergeSort(sorted);
        //-----------------------------------------------------------------------------------//

        const std::chrono::steady_clock::time_point t_wc1 = std::chrono::steady_clock::now();

        //-----------------------------------------------------------------------------------//
        // Task 1.2: Character splitting.
        //-----------------------------------------------------------------------------------//
        results.word_counts.resize(sorted.size());
        results.char_splits.resize(sorted.size());

        #pragma omp parallel for
        for(std::size_t i = 0; i < sorted.size(); i++)
        {
            const std::pair<const bpe::Byte*, std::size_t>& entry = sorted[i];
            const bpe::Byte* s = entry.first;
            const std::size_t n = std::strlen(reinterpret_cast<const char*>(s));
            const std::vector<bpe::Byte> bytes(s, s + n);
            results.word_counts[i] = (bpe::WordCount{bytes, entry.second});
            results.char_splits[i] = (bpe::CharSplit{bytes, entry.second});
        }
        //-----------------------------------------------------------------------------------//

        const std::chrono::steady_clock::time_point t_cs1 = std::chrono::steady_clock::now();

        LOG(INFO) << "word count: " << elapsed_ms(t_wc0, t_wc1) << " ms";
        LOG(INFO) << "char split: " << elapsed_ms(t_wc1, t_cs1) << " ms";
    }
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //