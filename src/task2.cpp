// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Dependencies
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "bpe.h"

#include "task2/task2_defs.h"
#include "task2/task2_build_state.h"
#include "task2/task2_run_merge_loop.h"

// Standard library.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <array>

#include <omp.h>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
namespace bpe
{
    namespace
    {
        //-----------------------------------------------------------------------------------//
        // Original build_state.
        //-----------------------------------------------------------------------------------//
        /*
        void build_state(const std::vector<CharSplit>& splits, task2::task2_state& state)
        {
            if (splits.size() >= task2::no_position)
            {
                throw std::length_error("too many distinct words");
            }

            state.vocabulary.resize(task2::byte_value_count);
            state.token_count.assign(task2::byte_value_count, 0);
            for (task2::u32 value = 1; value < task2::byte_value_count; ++value)
            {
                state.vocabulary[value].assign(1, static_cast<char>(value));
            }

            state.word_frequencies.reserve(splits.size());
            std::size_t slot_count = 0;
            for (const CharSplit& split : splits)
            {
                slot_count += split.chars.size() + 1;
            }
            if (slot_count >= task2::no_position)
            {
                throw std::length_error("input has too many byte positions");
            }
            state.token.reserve(slot_count);
            state.previous.reserve(slot_count);
            state.next.reserve(slot_count);
            state.word_of.reserve(slot_count);
            state.edge_group.reserve(slot_count);
            state.alive.reserve(slot_count);
            state.group_state.reserve(slot_count);
            state.group_count.reserve(slot_count);

            std::vector<task2::u32> initial_state(task2::byte_value_count * task2::byte_value_count, task2::no_position);
            for (task2::u32 word = 0; word < splits.size(); ++word)
            {
                const CharSplit& split = splits[word];
                state.word_frequencies.push_back(split.count);
                const task2::u32 first = static_cast<task2::u32>(state.token.size());

                for (std::size_t index = 0; index < split.chars.size(); ++index)
                {
                    const task2::u32 position = static_cast<task2::u32>(state.token.size());
                    const task2::u32 value = split.chars[index];
                    if (value == 0)
                    {
                        throw std::invalid_argument("word contains a NUL byte");
                    }
                    state.token.push_back(value);
                    state.previous.push_back(index == 0 ? task2::no_position : position - 1);
                    state.next.push_back(position + 1);
                    state.word_of.push_back(word);
                    state.edge_group.push_back(task2::no_position);
                    state.alive.push_back(1);
                    state.token_count[value] += split.count;
                }

                const task2::u32 sentinel = static_cast<task2::u32>(state.token.size());
                state.token.push_back(0);
                state.previous.push_back(split.chars.empty() ? task2::no_position : sentinel - 1);
                state.next.push_back(task2::no_position);
                state.word_of.push_back(word);
                state.edge_group.push_back(task2::no_position);
                state.alive.push_back(0);

                if (split.chars.empty())
                {
                    continue;
                }
                for (task2::u32 position = first; is_live(state, position); position = state.next[position])
                {
                    const task2::u32 next_position = state.next[position];
                    if (!is_live(state, next_position))
                    {
                        break;
                    }
                    const task2::u32 left = state.token[position];
                    const task2::u32 right = state.token[next_position];
                    task2::u32& state_id = initial_state[left * task2::byte_value_count + right];
                    if (state_id == task2::no_position)
                    {
                        state_id = create_pair_state(state, task2::pack_pair(left, right));
                    }

                    task2::pair_state& pair = state.pair_states[state_id];
                    task2::u32 group = task2::no_position;
                    if (pair.last_word == word)
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
                    pair.count += split.count;
                    pair.positions.push_back(position);
                    state.edge_group[position] = group;
                }
            }

            state.born_states.clear();
            const std::size_t cache_size = state.vocabulary.size() + 1024;
            state.left_stamp.assign(cache_size, task2::no_position);
            state.left_state.assign(cache_size, task2::no_position);
            state.right_stamp.assign(cache_size, task2::no_position);
            state.right_state.assign(cache_size, task2::no_position);
        }
        */
        //-----------------------------------------------------------------------------------//

        //-----------------------------------------------------------------------------------//
        // Original run_merge_loop.
        //-----------------------------------------------------------------------------------//
        /*
        void run_merge_loop(task2::task2_state& state)
        {
            task2::queue_heap queue;
            queue.comp.state = &state;
            for (task2::u32 state_id = 0; state_id < state.pair_states.size(); ++state_id)
            {
                const task2::pair_state& pair = state.pair_states[state_id];
                if (pair.word_count >= 2)
                {
                    queue.push(task2::queue_entry{pair.count, pair.fingerprint, state_id});
                }
            }

            for (;;)
            {
                const task2::u32 best_state = pop_best_state(state, queue);
                if (best_state == task2::no_position)
                {
                    break;
                }

                const task2::u64 best_key = state.pair_states[best_state].key;
                std::vector<task2::u32> positions = std::move(state.pair_states[best_state].positions);
                const task2::u32 left_token = task2::pair_left(best_key);
                const task2::u32 right_token = task2::pair_right(best_key);
                const task2::u32 merged_token = static_cast<task2::u32>(state.vocabulary.size());

                std::string merged_text;
                merged_text.reserve(state.vocabulary[left_token].size() + state.vocabulary[right_token].size());
                merged_text.append(state.vocabulary[left_token]);
                merged_text.append(state.vocabulary[right_token]);
                state.vocabulary.push_back(std::move(merged_text));
                state.token_count.push_back(0);
                if (state.left_stamp.size() <= merged_token)
                {
                    const std::size_t new_size = std::max<std::size_t>(state.left_stamp.size() * 2, merged_token + 1024);
                    state.left_stamp.resize(new_size, task2::no_position);
                    state.left_state.resize(new_size, task2::no_position);
                    state.right_stamp.resize(new_size, task2::no_position);
                    state.right_state.resize(new_size, task2::no_position);
                }

                task2::u32 current_word = task2::no_position;
                task2::u64 frequency = 0;
                for (task2::u32 position : positions)
                {
                    if (!pair_is_at(state, position, left_token, right_token))
                    {
                        continue;
                    }
                    const task2::u32 right_position = state.next[position];
                    const task2::u32 word = state.word_of[position];
                    if (word != current_word)
                    {
                        current_word = word;
                        frequency = state.word_frequencies[word];
                    }
                    const task2::u32 left_position = state.previous[position];
                    const task2::u32 after_position = state.next[right_position];

                    if (is_live(state, left_position))
                    {
                        remove_edge(state, left_position, frequency);
                    }
                    remove_edge(state, position, frequency);
                    if (is_live(state, after_position))
                    {
                        remove_edge(state, right_position, frequency);
                    }

                    state.token_count[left_token] -= frequency;
                    state.token_count[right_token] -= frequency;
                    state.token_count[merged_token] += frequency;
                    state.token[position] = merged_token;
                    state.alive[right_position] = 0;
                    state.next[position] = after_position;
                    if (after_position != task2::no_position)
                    {
                        state.previous[after_position] = position;
                    }
                    state.previous[right_position] = task2::no_position;
                    state.next[right_position] = task2::no_position;
                    state.edge_group[right_position] = task2::no_position;

                    if (is_live(state, left_position))
                    {
                        const task2::u32 state_id = get_left_pair_state(state, state.token[left_position], merged_token);
                        add_edge(state, left_position, state_id, word, frequency);
                    }
                    if (is_live(state, after_position))
                    {
                        const task2::u32 state_id = get_right_pair_state(state, merged_token, state.token[after_position]);
                        add_edge(state, position, state_id, word, frequency);
                    }
                }
                push_born_states(state, queue);
            }
        }
        */
        //-----------------------------------------------------------------------------------//

        //-----------------------------------------------------------------------------------//
        // Original finalize_results.
        //-----------------------------------------------------------------------------------//
        /*
        void finalize_results(const task2::task2_state& state, bpe::Results& results)
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
            std::sort
            (
                live_tokens.begin(),
                live_tokens.end(),
                [&state](task2::u32 left, task2::u32 right)
                {
                    if (state.token_count[left] != state.token_count[right])
                    {
                        return state.token_count[left] > state.token_count[right];
                    }
                    return std::strcmp(state.vocabulary[left].c_str(), state.vocabulary[right].c_str()) < 0;
                }
            );

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
        */
        //-----------------------------------------------------------------------------------//

        //---------------------------------------------------------------------------------------//
        void finalize_results(const Merge& merge, bpe::Results& results)
        {
            std::vector<task2::u64> token_count(merge.m_vocabulary.size(), 0);

            //-----------------------------------------------------------------------------------//
            // Calculate final token counts.
            //-----------------------------------------------------------------------------------//
            for(task2::u32 word = 0; word < merge.m_words.size(); word++)
            {
                for(task2::u32 token : merge.m_words[word])
                {
                    token_count[token] += merge.m_word_frequencies[word];
                }
            }
            //-----------------------------------------------------------------------------------//

            //-----------------------------------------------------------------------------------//
            // Find tokens that actually occur.
            //-----------------------------------------------------------------------------------//
            std::vector<task2::u32> live_tokens;
            live_tokens.reserve(token_count.size());

            for(task2::u32 token_id = 0; token_id < token_count.size(); token_id++)
            {
                if(token_count[token_id] != 0)
                {
                    live_tokens.push_back(token_id);
                }
            }
            //-----------------------------------------------------------------------------------//

            //-----------------------------------------------------------------------------------//
            // Sort by decreasing count, then lexicographically.
            //-----------------------------------------------------------------------------------//
            std::sort
            (
                live_tokens.begin(),
                live_tokens.end(),
                [&merge, &token_count](task2::u32 left, task2::u32 right)
                {
                    if(token_count[left] != token_count[right])
                    {
                        return token_count[left] > token_count[right];
                    }

                    return std::strcmp
                    (
                        merge.m_vocabulary[left].c_str(),
                        merge.m_vocabulary[right].c_str()
                    ) < 0;
                }
            );
            //-----------------------------------------------------------------------------------//

            //-----------------------------------------------------------------------------------//
            // Create results.
            //-----------------------------------------------------------------------------------//
            results.tokens.clear();
            results.tokens.reserve(live_tokens.size());

            for(task2::u32 token_id : live_tokens)
            {
                const std::string& text = merge.m_vocabulary[token_id];

                results.tokens.push_back
                (
                    bpe::TokenCount
                    {
                        std::vector<bpe::Byte>(text.begin(), text.end()),
                        static_cast<std::size_t>(token_count[token_id])
                    }
                );
            }
            //-----------------------------------------------------------------------------------//
        }
        //---------------------------------------------------------------------------------------//
    }

    // task2: greedy BPE — repeatedly merge the most frequent adjacent pair.
    void task2(const std::vector<CharSplit>& splits, Results& results)
    {
        task2::task2_state state;

        BuildState build_state;
        build_state.build_state(splits, state);
        
        Merge merge(state);
        merge.run();

        finalize_results(merge, results);
    }
}
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //