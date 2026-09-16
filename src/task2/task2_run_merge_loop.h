// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef TASK2_RUN_MERGE_LOOP_H
#define TASK2_RUN_MERGE_LOOP_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../bpe.h"
#include "task2_defs.h"

// Third party.

// Standard library.
#include <unordered_map>
#include <unordered_set>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct Merge
{
    //---------------------------------------------------------------------------------------//
    struct WordTokens
    {
        std::vector<task2::u32> tokens;
    };

    struct Pair
    {
        task2::u32 left;
        task2::u32 right;
        task2::u64 count;
        task2::u32 word_count;
    };

    struct PairCount
    {
        task2::u64 count;
        task2::u32 word_count;
    };
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    std::vector<WordTokens> m_words;
    Pair                                      m_best_pair;
    std::unordered_map<task2::u64, PairCount> m_pair_counts;
    bool                                      m_has_best_pair = false;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void initialise(const task2::task2_state& state)
    {
        m_words.resize(state.word_frequencies.size());

        for(task2::u32 word = 0; word < m_words.size(); ++word)
        {
            //-----------------------------------------------------------------------------------//
            // Find the first token belonging to this word.
            //-----------------------------------------------------------------------------------//
            task2::u32 position = task2::no_position;

            for(task2::u32 candidate = 0; candidate < state.token.size(); ++candidate)
            {
                if
                (
                    state.word_of[candidate] == word &&
                    state.alive[candidate] != 0 &&
                    state.previous[candidate] == task2::no_position
                )
                {
                    position = candidate;
                    break;
                }
            }
            //-----------------------------------------------------------------------------------//

            //-----------------------------------------------------------------------------------//
            // Copy the linked-list tokens into our contiguous word representation.
            //-----------------------------------------------------------------------------------//
            while(position != task2::no_position)
            {
                m_words[word].tokens.push_back(state.token[position]);

                position = state.next[position];

                if(position != task2::no_position && state.alive[position] == 0)
                {
                    break;
                }
            }
            //-----------------------------------------------------------------------------------//
        }
    }

    void countPairs(const task2::task2_state& state)
    {
        m_pair_counts.clear();

        for(task2::u32 word = 0; word < m_words.size(); ++word)
        {
            const std::vector<task2::u32>& tokens = m_words[word].tokens;
            const task2::u64 frequency = state.word_frequencies[word];

            std::unordered_set<task2::u64> word_pairs;

            for(std::size_t index = 0; index + 1 < tokens.size(); ++index)
            {
                const task2::u32 left  = tokens[index];
                const task2::u32 right = tokens[index + 1];

                const task2::u64 key = task2::pack_pair(left, right);

                m_pair_counts[key].count += frequency;
                word_pairs.insert(key);
            }

            for(const task2::u64 key : word_pairs)
            {
                ++m_pair_counts[key].word_count;
            }
        }
    }

    void selectBestPair(const task2::task2_state& state)
    {
        m_has_best_pair = false;

        task2::u64 best_count = 0;
        std::string best_string;

        for(const auto& entry : m_pair_counts)
        {
            const task2::u64 key = entry.first;
            const PairCount& pair = entry.second;

            if(pair.word_count < 2)
            {
                continue;
            }

            const task2::u32 left  = task2::pair_left(key);
            const task2::u32 right = task2::pair_right(key);

            std::string merged_string;
            merged_string.reserve(state.vocabulary[left].size() + state.vocabulary[right].size());

            merged_string.append(state.vocabulary[left]);
            merged_string.append(state.vocabulary[right]);

            if
            (
                m_has_best_pair == false ||
                pair.count > best_count ||
                (
                    pair.count == best_count &&
                    std::strcmp(merged_string.c_str(), best_string.c_str()) < 0
                )
            )
            {
                m_best_pair.left       = left;
                m_best_pair.right      = right;
                m_best_pair.count      = pair.count;
                m_best_pair.word_count = pair.word_count;

                best_count = pair.count;
                best_string = std::move(merged_string);
                m_has_best_pair = true;
            }
        }
    }

    void mergePair(task2::task2_state& state)
    {
        const task2::u32 left  = m_best_pair.left;
        const task2::u32 right = m_best_pair.right;
        const task2::u32 merged = static_cast<task2::u32>(state.vocabulary.size());

        std::string merged_string;
        merged_string.reserve(
            state.vocabulary[left].size() +
            state.vocabulary[right].size()
        );

        merged_string.append(state.vocabulary[left]);
        merged_string.append(state.vocabulary[right]);

        state.vocabulary.push_back(std::move(merged_string));

        //-----------------------------------------------------------------------------------//
        // Merge the selected pair in every word.
        //-----------------------------------------------------------------------------------//
        for(WordTokens& word : m_words)
        {
            std::vector<task2::u32> tokens;
            tokens.reserve(word.tokens.size());

            for(std::size_t index = 0; index < word.tokens.size();)
            {
                if
                (
                    index + 1 < word.tokens.size() &&
                    word.tokens[index]     == left &&
                    word.tokens[index + 1] == right
                )
                {
                    tokens.push_back(merged);
                    index += 2;
                }
                else
                {
                    tokens.push_back(word.tokens[index]);
                    ++index;
                }
            }

            word.tokens = std::move(tokens);
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void run_merge_loop(task2::task2_state& state)
    {
        initialise(state);

        for(;;)
        {
            countPairs(state);
            selectBestPair(state);

            if(m_has_best_pair == false)
            {
                break;
            }

            mergePair(state);
        }
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //