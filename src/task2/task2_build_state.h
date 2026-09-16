// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef TASK2_BUILD_STATE_H
#define TASK2_BUILD_STATE_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "task2_defs.h"

// Third party.
#include <omp.h>

// Standard library.
#include <array>
#include <stdexcept>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct BuildState
{
    //---------------------------------------------------------------------------------------//
    bool checkSplitsContainNull(const std::vector<bpe::CharSplit>& splits)
    {
        for(const bpe::CharSplit& split : splits)
        {
            for(const bpe::Byte value : split.chars)
            {
                if(value == 0)
                {
                    return false;
                }
            }
        }
        return true;
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void initialiseVocabulary(task2::task2_state& state)
    {
        state.vocabulary.resize(task2::byte_value_count);
        state.token_count.assign(task2::byte_value_count, 0);
        for(task2::u32 value = 1; value < task2::byte_value_count; ++value)
        {
            state.vocabulary[value].assign(1, static_cast<char>(value));
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    bool calcWordStarts
    (
        const std::vector<bpe::CharSplit>& splits           ,
        task2::task2_state&                state            ,
        std::vector<task2::u32>&           output_word_start,
        std::size_t&                       output_slot_count
    )
    {
        output_word_start.resize(splits.size());

        output_slot_count = 0;

        for(std::size_t i = 0; i < splits.size(); i++)
        {
            output_word_start[i] = static_cast<task2::u32>(output_slot_count);
            output_slot_count += splits[i].chars.size() + 1;
        }
        if(output_slot_count >= task2::no_position)
        {
            return false;
        }

        state.word_frequencies.resize(splits.size());
        state.token.resize(output_slot_count);
        state.previous.resize(output_slot_count);
        state.next.resize(output_slot_count);
        state.word_of.resize(output_slot_count);
        state.edge_group.resize(output_slot_count);
        state.alive.resize(output_slot_count);

        return true;
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void buildTokens
    (
        const std::vector<bpe::CharSplit>& splits    ,
        const std::vector<task2::u32>&     word_start,
        task2::task2_state&                state
    )
    {
        const int num_threads = omp_get_max_threads();
        std::vector<std::array<task2::u64, task2::byte_value_count>> thread_token_counts(num_threads);
        for(auto& counts : thread_token_counts)
        {
            counts.fill(0);
        }

        #pragma omp parallel
        {
            const int thread_index = omp_get_thread_num();
            auto& local_counts = thread_token_counts[thread_index];

            #pragma omp for
            for(task2::u32 word = 0; word < splits.size(); word++)
            {
                const bpe::CharSplit& split = splits[word];

                state.word_frequencies[word] = split.count;
                const task2::u32 first = word_start[word];

                for(std::size_t index = 0; index < split.chars.size(); index++)
                {
                    const task2::u32 position = first + static_cast<task2::u32>(index);
                    const task2::u32 value = split.chars[index];

                    state.token[position]       = value;
                    state.previous[position]    = index == 0 ? task2::no_position : position - 1;
                    state.next[position]        = position + 1;
                    state.word_of[position]     = word;
                    state.edge_group[position]  = task2::no_position;
                    state.alive[position]       = 1;

                    local_counts[value] += split.count;
                }

                const task2::u32 sentinel = first + static_cast<task2::u32>(split.chars.size());

                state.token[sentinel]      = 0;
                state.previous[sentinel]   = split.chars.empty() ? task2::no_position : sentinel - 1;
                state.next[sentinel]       = task2::no_position;
                state.word_of[sentinel]    = word;
                state.edge_group[sentinel] = task2::no_position;
                state.alive[sentinel]      = 0;
            }
        }

        state.token_count.assign(task2::byte_value_count, 0);
        for(int thread_index = 0; thread_index < num_threads; thread_index++)
        {
            for(task2::u32 value = 0; value < task2::byte_value_count; value++)
            {
                state.token_count[value] += thread_token_counts[thread_index][value];
            }
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    struct LocalPair
    {
        task2::u32 left;
        task2::u32 right;
        task2::u32 word;
        task2::u32 position;
        task2::u64 frequency;
    };

    std::vector<std::vector<LocalPair>> buildLocalPairs
    (
        const std::vector<bpe::CharSplit>& splits    ,
        const std::vector<task2::u32>&     word_start,
        const task2::task2_state&          state
    )
    {
        const int num_threads = omp_get_max_threads();
        std::vector<std::vector<LocalPair>> thread_pairs(num_threads);

        #pragma omp parallel
        {
            const int thread_index = omp_get_thread_num();
            auto& local_pairs = thread_pairs[thread_index];

            #pragma omp for
            for(task2::u32 word = 0; word < splits.size(); word++)
            {
                const bpe::CharSplit& split = splits[word];

                if(split.chars.empty())
                {
                    continue;
                }

                const task2::u32 first = word_start[word];

                for(task2::u32 position = first; is_live(state, position); position = state.next[position])
                {
                    const task2::u32 next_position = state.next[position];
                    if(!is_live(state, next_position))
                    {
                        break;
                    }

                    const task2::u32 left = state.token[position];
                    const task2::u32 right = state.token[next_position];

                    local_pairs.push_back(LocalPair{left, right, word, position, split.count});
                }
            }
        }

        return thread_pairs;
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void build_state(const std::vector<bpe::CharSplit>& splits, task2::task2_state& state)
    {
        if(splits.size() >= task2::no_position) { throw std::length_error("too many distinct words"); }

        bool check_success = checkSplitsContainNull(splits);
        if(check_success == false)
        {
            throw std::invalid_argument("word contains a NUL byte");
        }

        this->initialiseVocabulary(state);

        std::vector<task2::u32> word_start;
        std::size_t             slot_count;
        check_success = calcWordStarts(splits, state, word_start, slot_count);
        if(check_success == false)
        {
            throw std::length_error("input has too many byte positions");
        }

        //-----------------------------------------------------------------------------------//
        // Parallel section.
        //-----------------------------------------------------------------------------------//
        buildTokens(splits, word_start, state);
        //-----------------------------------------------------------------------------------//

        //-----------------------------------------------------------------------------------//
        // Parallel section.
        //-----------------------------------------------------------------------------------//
        std::vector<std::vector<LocalPair>> thread_pairs = buildLocalPairs(splits, word_start, state);
        //-----------------------------------------------------------------------------------//

        //-----------------------------------------------------------------------------------//
        std::vector<task2::u32> initial_state(task2::byte_value_count * task2::byte_value_count, task2::no_position);

        // Number of elements is determined dynamically.
        state.group_state.reserve(slot_count);
        state.group_count.reserve(slot_count);

        for(size_t thread_index = 0; thread_index < thread_pairs.size(); thread_index++)
        {
            for(const LocalPair& local_pair : thread_pairs[thread_index])
            {
                const task2::u32 left     = local_pair.left    ;
                const task2::u32 right    = local_pair.right   ;
                const task2::u32 word     = local_pair.word    ;
                const task2::u32 position = local_pair.position;

                task2::u32& state_id = initial_state[left * task2::byte_value_count + right];
                if(state_id == task2::no_position)
                {
                    state_id = create_pair_state(state, task2::pack_pair(left, right));
                }

                task2::pair_state& pair = state.pair_states[state_id];
                task2::u32 group = task2::no_position;

                if(pair.last_word == word)
                {
                    group = pair.last_group;
                }
                else
                {
                    group = static_cast<task2::u32>(state.group_state.size());

                    state.group_state.push_back(state_id);
                    state.group_count.push_back(0);

                    pair.last_word = word;
                    pair.last_group = group;

                    ++pair.word_count;
                }

                ++state.group_count[group];

                pair.count += local_pair.frequency;
                pair.positions.push_back(position);

                state.edge_group[position] = group;
            }
        }
        //-----------------------------------------------------------------------------------//

        state.born_states.clear();
        const std::size_t cache_size = state.vocabulary.size() + 1024;
        state.left_stamp.assign(cache_size, task2::no_position);
        state.left_state.assign(cache_size, task2::no_position);
        state.right_stamp.assign(cache_size, task2::no_position);
        state.right_state.assign(cache_size, task2::no_position);
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //