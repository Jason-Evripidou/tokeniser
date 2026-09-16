// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef TASK2_DEFS_H
#define TASK2_DEFS_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../bpe.h"
#include "task2_defs.h"

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
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
namespace task2
{
    using u8 = std::uint8_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    constexpr u32 no_position = std::numeric_limits<u32>::max();
    constexpr u32 byte_value_count = 256;

    u64 pack_pair(u32 left, u32 right)
    {
        return (static_cast<u64>(left) << 32) | static_cast<u64>(right);
    }

    u32 pair_left(u64 key) { return static_cast<u32>(key >> 32); }

    u32 pair_right(u64 key) { return static_cast<u32>(key); }

    u64 text_fingerprint(const std::string& left, const std::string& right)
    {
        u64 fingerprint = 0;
        std::size_t i = 0;
        const std::size_t n = std::min<std::size_t>(left.size(), 8);
        for (; i < n; ++i)
        {
            fingerprint |= static_cast<u64>(static_cast<u8>(left[i])) << (8 * (7 - i));
        }
        const std::size_t m = std::min<std::size_t>(right.size(), 8 - i);
        for (std::size_t j = 0; j < m; ++j, ++i)
        {
            fingerprint |= static_cast<u64>(static_cast<u8>(right[j])) << (8 * (7 - i));
        }
        return fingerprint;
    }

    //-----------------------------------------------------------------------------------//
    /*
    -   pair_state represents a unique adjacent token pair.
    -   key is compromised of two u32 token IDs. It defines the pair of tokens.
    -   count is the total frequency of this pair across all words/tokens.
    -   word_count is the number of distinct words/tokens containing the pair.
    -   positions is the positions where this pair currently occurs across the globally
    |   state.token array.
    -   last_word cache of the token id of the last word/token for which this pair's group
    |   was accessed.
    -   last_group group ID corresponding to last_word.
    -   fingerprint 64-bit representation of the pair's text.
    */
    //-----------------------------------------------------------------------------------//
    struct pair_state
    {
        u64 key = 0;
        u64 count = 0;
        u32 word_count = 0;
        std::vector<u32> positions;
        u32 last_word = no_position;
        u32 last_group = no_position;
        u64 fingerprint = 0;
    };
    //-----------------------------------------------------------------------------------//

    struct queue_entry
    {
        u64 count = 0;
        u64 fingerprint = 0;
        u32 state = 0;
    };

    struct task2_state;

    struct queue_compare
    {
        const task2_state* state = nullptr;

        bool operator()(const queue_entry& left, const queue_entry& right) const;
    };

    struct queue_heap
    {
        std::vector<queue_entry> data;
        queue_compare comp;

        bool empty() const { return data.empty(); }

        const queue_entry& top() const { return data.front(); }

        void push(queue_entry entry)
        {
            data.push_back(entry);
            sift_up(data.size() - 1);
        }

        void pop()
        {
            data[0] = std::move(data.back());
            data.pop_back();
            if (!data.empty())
            {
                sift_down(0);
            }
        }

    private:
        static std::size_t parent(std::size_t i) { return (i - 1) / 4; }
        static std::size_t first_child(std::size_t i) { return 4 * i + 1; }

        void sift_up(std::size_t i)
        {
            while (i > 0)
            {
                const std::size_t p = parent(i);
                if (!comp(data[p], data[i]))
                {
                    break;
                }
                std::swap(data[p], data[i]);
                i = p;
            }
        }

        void sift_down(std::size_t i)
        {
            for (;;)
            {
                const std::size_t fc = first_child(i);
                if (fc >= data.size())
                {
                    break;
                }
                std::size_t best = fc;
                const std::size_t end = std::min(fc + 4, data.size());
                for (std::size_t c = fc + 1; c < end; ++c)
                {
                    if (comp(data[best], data[c]))
                    {
                        best = c;
                    }
                }
                if (!comp(data[i], data[best]))
                {
                    break;
                }
                std::swap(data[i], data[best]);
                i = best;
            }
        }
    };

    struct task2_state
    {
        std::vector<u64> word_frequencies;
        std::vector<u32> token;
        std::vector<u32> previous;
        std::vector<u32> next;
        std::vector<u32> word_of;
        std::vector<u32> edge_group;
        std::vector<u8> alive;
        std::vector<u64> token_count;
        std::vector<std::string> vocabulary;
        std::vector<pair_state> pair_states;
        std::vector<u32> group_state;
        std::vector<u32> group_count;
        std::vector<u32> left_stamp;
        std::vector<u32> left_state;
        std::vector<u32> right_stamp;
        std::vector<u32> right_state;
        u32 same_stamp = no_position;
        u32 same_state = no_position;
        std::vector<u32> born_states;
        // dead groups are reused; a dead group's group_count holds the next index
        u32 free_head = no_position;
    };

    bool queue_compare::operator()(const queue_entry& left, const queue_entry& right) const
    {
        if (left.count != right.count)
        {
            return left.count < right.count;
        }
        if (left.fingerprint != right.fingerprint)
        {
            return left.fingerprint > right.fingerprint;
        }
        const pair_state& a = state->pair_states[left.state];
        const pair_state& b = state->pair_states[right.state];
        const std::string left_text = state->vocabulary[pair_left(a.key)] + state->vocabulary[pair_right(a.key)];
        const std::string right_text = state->vocabulary[pair_left(b.key)] + state->vocabulary[pair_right(b.key)];
        return std::strcmp(left_text.c_str(), right_text.c_str()) > 0;
    }

    bool is_live(const task2_state& state, u32 position)
    {
        return position != no_position && state.alive[position] != 0;
    }

    u32 create_pair_state(task2_state& state, u64 key)
    {
        const u32 state_id = static_cast<u32>(state.pair_states.size());
        state.pair_states.push_back(pair_state{});
        pair_state& pair = state.pair_states.back();
        pair.key = key;
        pair.fingerprint = text_fingerprint(state.vocabulary[pair_left(key)], state.vocabulary[pair_right(key)]);
        state.born_states.push_back(state_id);
        return state_id;
    }

    u32 get_left_pair_state(task2_state& state, u32 left, u32 merged)
    {
        if (left == merged)
        {
            if (state.same_stamp != merged)
            {
                state.same_stamp = merged;
                state.same_state = create_pair_state(state, pack_pair(merged, merged));
            }
            return state.same_state;
        }
        if (state.left_stamp[left] != merged)
        {
            state.left_stamp[left] = merged;
            state.left_state[left] = create_pair_state(state, pack_pair(left, merged));
        }
        return state.left_state[left];
    }

    u32 get_right_pair_state(task2_state& state, u32 merged, u32 right)
    {
        if (right == merged)
        {
            if (state.same_stamp != merged)
            {
                state.same_stamp = merged;
                state.same_state = create_pair_state(state, pack_pair(merged, merged));
            }
            return state.same_state;
        }
        if (state.right_stamp[right] != merged)
        {
            state.right_stamp[right] = merged;
            state.right_state[right] = create_pair_state(state, pack_pair(merged, right));
        }
        return state.right_state[right];
    }

    u32 get_word_group(task2_state& state, u32 state_id, u32 word)
    {
        pair_state& pair = state.pair_states[state_id];
        if (pair.last_word == word)
        {
            return pair.last_group;
        }
        u32 group;
        if (state.free_head != no_position)
        {
            group = state.free_head;
            state.free_head = state.group_count[group];
            state.group_count[group] = 0;
            state.group_state[group] = state_id;
        }
        else
        {
            group = static_cast<u32>(state.group_state.size());
            state.group_state.push_back(state_id);
            state.group_count.push_back(0);
        }
        pair.last_word = word;
        pair.last_group = group;
        return group;
    }

    void remove_edge(task2_state& state, u32 start, u64 frequency)
    {
        const u32 group = state.edge_group[start];
        if (group == no_position || state.group_count[group] == 0)
        {
            std::abort();
        }
        pair_state& pair = state.pair_states[state.group_state[group]];
        --state.group_count[group];
        if (pair.count < frequency)
        {
            std::abort();
        }
        pair.count -= frequency;
        if (state.group_count[group] == 0)
        {
            if (pair.word_count == 0)
            {
                std::abort();
            }
            --pair.word_count;
            if (pair.last_group == group)
            {
                pair.last_group = no_position;
                pair.last_word = no_position;
            }
            state.group_count[group] = state.free_head;
            state.free_head = group;
        }
        state.edge_group[start] = no_position;
    }

    void add_edge(task2_state& state, u32 start, u32 state_id, u32 word, u64 frequency)
    {
        const u32 group = get_word_group(state, state_id, word);
        pair_state& pair = state.pair_states[state_id];
        if (state.group_count[group]++ == 0)
        {
            ++pair.word_count;
        }
        pair.count += frequency;
        pair.positions.push_back(start);
        state.edge_group[start] = group;
    }

    bool pair_is_at(const task2_state& state, u32 position, u32 left, u32 right)
    {
        if (!is_live(state, position) || state.token[position] != left)
        {
            return false;
        }
        const u32 next_position = state.next[position];
        return is_live(state, next_position) && state.token[next_position] == right;
    }

    void push_born_states(task2_state& state, queue_heap& queue)
    {
        for (u32 state_id : state.born_states)
        {
            const pair_state& pair = state.pair_states[state_id];
            if (pair.word_count >= 2 && pair.count != 0)
            {
                queue.push(queue_entry{pair.count, pair.fingerprint, state_id});
            }
        }
        state.born_states.clear();
    }

    u32 pop_best_state(task2_state& state, queue_heap& queue)
    {
        while (!queue.empty())
        {
            const queue_entry entry = queue.top();
            queue.pop();
            const pair_state& pair = state.pair_states[entry.state];
            if (pair.word_count < 2 || pair.count == 0)
            {
                continue;
            }
            if (entry.count != pair.count)
            {
                queue.push(queue_entry{pair.count, pair.fingerprint, entry.state});
                continue;
            }
            return entry.state;
        }
        return no_position;
    }
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //