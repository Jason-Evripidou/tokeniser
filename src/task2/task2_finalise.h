// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef TASK2_FINALISE_H
#define TASK2_FINALISE_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#include "task2_defs.h"

#include <algorithm>
#include <cstring>
#include <vector>
#include <omp.h>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct Finalise
{
    inline bool less
    (
        task2::u32                left ,
        task2::u32                right,
        const task2::task2_state& state
    )
    {
        if(state.token_count[left] != state.token_count[right])
        {
            return state.token_count[left] > state.token_count[right];
        }

        return std::strcmp
        (
            state.vocabulary[left].c_str(),
            state.vocabulary[right].c_str()
        ) < 0;
    }

    inline void merge
    (
        std::vector<task2::u32>&  data  ,
        std::size_t               left  ,
        std::size_t               middle,
        std::size_t               right ,
        std::vector<task2::u32>&  temp  ,
        const task2::task2_state& state
    )
    {
        std::size_t left_index  = left;
        std::size_t right_index = middle + 1;
        std::size_t temp_index  = left;

        while((left_index <= middle) && (right_index <= right))
        {
            if(this->less(data[right_index], data[left_index], state))
            {
                temp[temp_index++] = data[right_index++];
            }
            else
            {
                temp[temp_index++] = data[left_index++];
            }
        }

        while(left_index <= middle)
        {
            temp[temp_index++] = data[left_index++];
        }

        while(right_index <= right)
        {
            temp[temp_index++] = data[right_index++];
        }

        for(std::size_t i = left; i <= right; ++i)
        {
            data[i] = temp[i];
        }
    }

    inline void mergeSortHelper
    (
        std::vector<task2::u32>&  data ,
        std::size_t               left ,
        std::size_t               right,
        std::vector<task2::u32>&  temp ,
        const task2::task2_state& state
    )
    {
        if(left >= right) { return; }

        const std::size_t middle = ((right - left) / 2) + left;

        this->mergeSortHelper(data, left, middle, temp, state);
        this->mergeSortHelper(data, middle + 1, right, temp, state);

        this->merge(data, left, middle, right, temp, state);
    }


    inline void parallelMergeSort(std::vector<task2::u32>& data, const task2::task2_state& state)
    {
        if(data.size() <= 1) { return; }

        std::vector<task2::u32> temp;
        temp.resize(data.size());

        const int num_threads = omp_get_max_threads();

        const std::size_t chunk_size = (data.size() + num_threads - 1) / num_threads;

        //-----------------------------------------------------------------------------------//
        // Sort the initial chunks in parallel.
        //-----------------------------------------------------------------------------------//

        #pragma omp parallel for
        for(int thread_index = 0; thread_index < num_threads; ++thread_index)
        {
            const std::size_t left = static_cast<std::size_t>(thread_index) * chunk_size;

            if(left >= data.size())
            {
                continue;
            }

            const std::size_t right = std::min(left + chunk_size, data.size()) - 1;

            this->mergeSortHelper(data, left, right, temp, state);
        }

        //-----------------------------------------------------------------------------------//
        // Merge the sorted chunks.
        //-----------------------------------------------------------------------------------//

        std::size_t current_chunk_size = chunk_size;

        while(current_chunk_size < data.size())
        {
            const std::size_t merge_size = current_chunk_size * 2;

            #pragma omp parallel for
            for(std::size_t left = 0; left < data.size(); left += merge_size)
            {
                const std::size_t middle = left + current_chunk_size - 1;

                if(middle >= data.size())
                {
                    continue;
                }

                const std::size_t right = std::min(left + merge_size - 1, data.size() - 1);

                this->merge(data, left, middle, right, temp, state);
            }

            current_chunk_size *= 2;
        }
    }

    inline void finalise(const task2::task2_state& state, bpe::Results& results)
    {
        std::vector<task2::u32> live_tokens;
        live_tokens.reserve(state.token_count.size());
        for (task2::u32 token_id = 1; token_id < state.token_count.size(); ++token_id)
        {
            if (state.token_count[token_id] != 0)
            {
                live_tokens.push_back(token_id);
            }
        }

        parallelMergeSort(live_tokens, state);

        results.tokens.clear();
        results.tokens.reserve(live_tokens.size());
        for (task2::u32 token_id : live_tokens)
        {
            const std::string& text = state.vocabulary[token_id];
            results.tokens.push_back
            (
                bpe::TokenCount{ std::vector<bpe::Byte>(text.begin(), text.end()), static_cast<std::size_t>(state.token_count[token_id]) }
            );
        }
    }
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //