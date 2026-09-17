// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef TASK2_RUN_MERGE_LOOP_H
#define TASK2_RUN_MERGE_LOOP_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../bpe.h"
#include "task2_defs.h"

// Third party.
#include <omp.h>

// Standard library.
#include <iostream>
#include <unordered_map>
#include <unordered_set>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct Pair
{
    task2::u64 key;
    bool operator==(const Pair& other) const
    {
        return (key == other.key);
    }
};
task2::u64 pack_pair(task2::u32 left, task2::u32 right)
{
    return (static_cast<task2::u64>(left) << 32) | static_cast<task2::u64>(right);
}
task2::u32 pair_left(task2::u64 key) { return static_cast<task2::u32>(key >> 32); }
task2::u32 pair_right(task2::u64 key) { return static_cast<task2::u32>(key); }

struct PairHash
{
    std::size_t operator()(const Pair& pair) const
    {
        return static_cast<std::size_t>(pair.key);
    }
};

struct PairCount
{
    task2::u64 count = 0;
    task2::u32 word_count = 0;
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct Merge
{
    //---------------------------------------------------------------------------------------//
    std::vector<std::unordered_map<Pair, PairCount, PairHash>> m_thread_pairs;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    std::vector<std::vector<task2::u32>> m_words;

    std::vector<std::string> m_vocabulary;

    std::vector<task2::u64> m_word_frequencies;

    std::unordered_map<Pair, PairCount, PairHash> m_pairs;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    Merge(const task2::task2_state& state)
    {
        const int num_threads = omp_get_max_threads();
        m_thread_pairs.resize(num_threads);

        m_words.resize(state.word_frequencies.size());
        m_vocabulary = std::move(state.vocabulary);
        m_word_frequencies = std::move(state.word_frequencies);

        for(task2::u32 word = 0; word < state.word_frequencies.size(); word++)
        {
            task2::u32 position = task2::no_position;

            for(task2::u32 candidate = 0; candidate < state.token.size(); candidate++)
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

            while(position != task2::no_position)
            {
                if(state.alive[position] == 0) { break; }

                m_words[word].push_back(state.token[position]);
                position = state.next[position];
            }
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void createPairs()
    {
        m_pairs.clear();

        for(task2::u32 word = 0; word < m_words.size(); word++)
        {
            std::unordered_set<Pair, PairHash> word_pairs;

            for(task2::u32 position = 0; position + 1 < m_words[word].size(); position++)
            {
                Pair pair;

                pair.key = pack_pair(m_words[word][position], m_words[word][position + 1]);

                PairCount& pair_count = m_pairs[pair];

                pair_count.count += m_word_frequencies[word];

                if(word_pairs.insert(pair).second)
                {
                    pair_count.word_count++;
                }
            }
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void createPairsParallel()
    {
        m_pairs.clear();

        for(std::unordered_map<Pair, PairCount, PairHash>& pairs : m_thread_pairs)
        {
            pairs.clear();
        }

        #pragma omp parallel
        {
            const int thread_id = omp_get_thread_num();
            std::unordered_map<Pair, PairCount, PairHash>& local_pairs = m_thread_pairs[thread_id];

            #pragma omp for
            for(task2::u32 word = 0; word < m_words.size(); word++)
            {
                std::unordered_set<Pair, PairHash> word_pairs;

                for(task2::u32 position = 0; position + 1 < m_words[word].size(); position++)
                {
                    Pair pair;
                    pair.key = pack_pair(m_words[word][position], m_words[word][position + 1]);

                    PairCount& pair_count = local_pairs[pair];
                    pair_count.count += m_word_frequencies[word];

                    if(word_pairs.insert(pair).second)
                    {
                        pair_count.word_count++;
                    }
                }
            }
        }

        for(const std::unordered_map<Pair, PairCount, PairHash>& local_pairs : m_thread_pairs)
        {
            for(const auto& entry : local_pairs)
            {
                const Pair& pair = entry.first;
                const PairCount& pair_count = entry.second;

                PairCount& total = m_pairs[pair];

                total.count      += pair_count.count;
                total.word_count += pair_count.word_count;
            }
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    bool selectBestPair(Pair& output_best_pair)
    {
        bool found_pair = false;

        task2::u64 best_count = 0;
        std::string best_string = std::string("");

        for(const auto& entry : m_pairs)
        {
            const Pair& pair = entry.first;
            const PairCount& pair_count = entry.second;

            if(pair_count.word_count < 2) { continue; }

            task2::u32 left  = static_cast<task2::u32>(pair.key >> 32);
            task2::u32 right = static_cast<task2::u32>(pair.key & 0xffffffffu);

            std::string merged_string = m_vocabulary[left] + m_vocabulary[right];

            if(!found_pair)
            {
                output_best_pair = pair;
                best_count       = pair_count.count;
                best_string      = merged_string;
                found_pair       = true;
                continue;
            }

            if(pair_count.count > best_count)
            {
                output_best_pair = pair;
                best_count       = pair_count.count;
                best_string      = merged_string;
            }
            else if
            (
                (pair_count.count == best_count) &&
                (merged_string < best_string)
            )
            {
                output_best_pair = pair;
                best_string      = merged_string;
            }
        }

        return found_pair;
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void mergePair(const Pair& pair)
    {
        task2::u32 left  = static_cast<task2::u32>(pair.key >> 32);
        task2::u32 right = static_cast<task2::u32>(pair.key & 0xffffffffu);

        task2::u32 merged_token = static_cast<task2::u32>(m_vocabulary.size());
        m_vocabulary.push_back(m_vocabulary[left] + m_vocabulary[right]);

        for(std::vector<task2::u32>& word : m_words)
        {
            std::vector<task2::u32> new_word;
            new_word.reserve(word.size());

            for(task2::u32 position = 0; position < word.size(); )
            {
                if
                (
                    (position + 1 < word.size())  &&
                    (word[position] == left)      &&
                    (word[position + 1] == right)
                )
                {
                    new_word.push_back(merged_token);
                    position += 2;
                }
                else
                {
                    new_word.push_back(word[position]);
                    position++;
                }
            }

            word = std::move(new_word);
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    void mergePairParallel(const Pair& pair)
    {
        task2::u32 left  = static_cast<task2::u32>(pair.key >> 32);
        task2::u32 right = static_cast<task2::u32>(pair.key & 0xffffffffu);

        task2::u32 merged_token = static_cast<task2::u32>(m_vocabulary.size());
        m_vocabulary.push_back(m_vocabulary[left] + m_vocabulary[right]);

        #pragma omp parallel for
        for(task2::u32 word_index = 0; word_index < m_words.size(); word_index++)
        {
            std::vector<task2::u32>& word = m_words[word_index];

            std::vector<task2::u32> new_word;
            new_word.reserve(word.size());

            for(task2::u32 position = 0; position < word.size(); )
            {
                if
                (
                    (position + 1 < word.size())  &&
                    (word[position] == left)      &&
                    (word[position + 1] == right)
                )
                {
                    new_word.push_back(merged_token);
                    position += 2;
                }
                else
                {
                    new_word.push_back(word[position]);
                    position++;
                }
            }

            word = std::move(new_word);
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Moved to task2.cpp.
    //---------------------------------------------------------------------------------------//
    /*
    void run()
    {
        while(true)
        {
            createPairs();
            //createPairsParallel();
            Pair best_pair;
            if(!selectBestPair(best_pair)) { break; }
            mergePair(best_pair);
            //mergePairParallel(best_pair);
        }
    }
    */
    //---------------------------------------------------------------------------------------//
 
    //---------------------------------------------------------------------------------------//
    // Debugging.
    //---------------------------------------------------------------------------------------//
    void printWords()
    {
        for(task2::u32 word = 0; word < m_words.size(); word++)
        {
            std::cout << "Word " << word << ": ";

            for(task2::u32 token : m_words[word])
            {
                std::cout << token << "('" << m_vocabulary[token] << "') ";
            }

            std::cout << std::endl;
        }
    }

    void printPairs()
    {
        for(const auto& entry : m_pairs)
        {
            const Pair& pair = entry.first;
            const PairCount& pair_count = entry.second;

            task2::u32 left  = static_cast<task2::u32>(pair.key >> 32);
            task2::u32 right = static_cast<task2::u32>(pair.key & 0xffffffffu);

            std::cout << "(" << left << ", " << right << ") '" << m_vocabulary[left] << m_vocabulary[right]
                << "'" << " count=" << pair_count.count << " words=" << pair_count.word_count << std::endl;
        }

        Pair best_pair;

        if(selectBestPair(best_pair))
        {
            task2::u32 left  = static_cast<task2::u32>(best_pair.key >> 32);
            task2::u32 right = static_cast<task2::u32>(best_pair.key & 0xffffffffu);

            std::cout
                << "Best pair: ("
                << left
                << ", "
                << right
                << ")\t\t'"
                << m_vocabulary[left]
                << m_vocabulary[right]
                << "'"
                << std::endl;
        }
        else
        {
            std::cout << "No eligible pair." << std::endl;
        }
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //