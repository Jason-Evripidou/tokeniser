// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
/*
-   A data structure to support concurrent access from multiple threads to count the frequency
|   of words. 
*/
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef WORD_COUNTS_H
#define WORD_COUNTS_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../bpe.h"

// Standard library.
#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
class WordCount
{
private:
    //---------------------------------------------------------------------------------------//
    // Internal data.
    //---------------------------------------------------------------------------------------//
    size_t m_count;
    mutable std::shared_mutex m_shared_mutex;
    //---------------------------------------------------------------------------------------//

public:
    //---------------------------------------------------------------------------------------//
    // Constructor and Destructor.
    //---------------------------------------------------------------------------------------//
    WordCount()
    :   m_count(0)
    {}

    WordCount(size_t count)
    :   m_count(count)
    {}
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    inline void resetCount()
    {
        std::unique_lock<std::shared_mutex> lock(m_shared_mutex);
        m_count = 0;
    }
    
    inline void increment()
    {
        std::unique_lock<std::shared_mutex> lock(m_shared_mutex);
        m_count++;
    }

    inline void decrement()
    {
        std::unique_lock<std::shared_mutex> lock(m_shared_mutex);
        if(m_count > 0)
        {
            m_count--;
        }
    }

    inline std::size_t getCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_shared_mutex);
        return m_count;
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
class WordCounts
{
private:
    //---------------------------------------------------------------------------------------//
    /*
    -   Following code copied from task1.cpp:
        -   Function: elapsed_ms()
        -   Function: hasless_any()
        -   struct ByteStrLess;
        -   struct ChunkedHash;
        -   struct ChunkedEq;
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

    static constexpr std::uint64_t kOnes = ~0ULL / 255;
    static inline std::uint64_t hasless_any(std::uint64_t x, unsigned limit)
    {
        return (x - kOnes * limit) & ~x & (kOnes * 128);
    }

    static inline std::uint64_t haszero(std::uint64_t x) { return hasless_any(x, 1); }

    struct ByteStrLess
    {
        bool operator()(const bpe::Byte* a, const bpe::Byte* b) const
        {
            std::size_t i = 0;
            while (a[i] != 0 && a[i] == b[i])
            {
                ++i;
            }
            return a[i] < b[i];
        }
    };

    struct ChunkedHash
    {
        const bpe::Byte* end;
        std::size_t operator()(const bpe::Byte* s) const
        {
            std::size_t h = 1469598103934665603ULL;
            const bpe::Byte* p = s;
            while (p + 8 <= end)
            {
                std::uint64_t c;
                std::memcpy(&c, p, 8);
                const std::uint64_t z = haszero(c);
                if (z)
                {
                    const std::size_t n = __builtin_ctzll(z) >> 3;
                    h ^= (c & ((1ULL << (8 * n)) - 1)) * 0x100000001b3ULL;
                    h ^= static_cast<std::size_t>(n + 1) * 0x100000001b3ULL;
                    return h;
                }
                h = (h ^ c) * 0x100000001b3ULL;
                p += 8;
            }

            const bpe::Byte* q = p;
            while (*q)
            {
                ++q;
            }
            const std::size_t n = static_cast<std::size_t>(q - p);
            std::uint64_t c = 0;
            std::memcpy(&c, p, n);
            h ^= (c & ((1ULL << (8 * n)) - 1)) * 0x100000001b3ULL;
            h ^= static_cast<std::size_t>(n + 1) * 0x100000001b3ULL;
            return h;
        }
    };

    struct ChunkedEq
    {
        const bpe::Byte* end;
        bool operator()(const bpe::Byte* a, const bpe::Byte* b) const
        {
            if (a == b)
            {
                return true;
            }
            const bpe::Byte* pa = a;
            const bpe::Byte* pb = b;
            while (pa + 8 <= end && pb + 8 <= end)
            {
                std::uint64_t ca, cb;
                std::memcpy(&ca, pa, 8);
                std::memcpy(&cb, pb, 8);
                const std::uint64_t z = haszero(ca) | haszero(cb);
                if (z)
                {
                    const std::size_t p = __builtin_ctzll(z) >> 3;
                    const std::uint64_t mask = (p == 7) ? ~std::uint64_t{0} : ((std::uint64_t{1} << (8 * (p + 1))) - 1);
                    return ((ca ^ cb) & mask) == 0;
                }
                if (ca != cb)
                {
                    return false;
                }
                pa += 8;
                pb += 8;
            }
            while (*pa && *pa == *pb)
            {
                ++pa;
                ++pb;
            }
            return *pa == *pb;
        }
    };
    //---------------------------------------------------------------------------------------//

private:
    //---------------------------------------------------------------------------------------//
    // External data. Must exist for lifetime of WordCounts object.
    //---------------------------------------------------------------------------------------//
    const std::vector<bpe::Word>& m_words;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Internal data.
    //---------------------------------------------------------------------------------------//
    mutable std::shared_mutex m_shared_mutex;
    //---------------------------------------------------------------------------------------//

public:
    //---------------------------------------------------------------------------------------//
    // Internal data.
    //---------------------------------------------------------------------------------------//
    std::unordered_map<const bpe::Byte*, WordCount, ChunkedHash, ChunkedEq> m_word_counts;
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Constructor and Destructor.
    //---------------------------------------------------------------------------------------//
    WordCounts(const std::vector<bpe::Word>& words)
    :   m_words(words)
    {
        const bpe::Byte* end = words.back().bytes;
        while(*end)
        {
            ++end;
        }
        ++end;

        m_word_counts = std::unordered_map<const bpe::Byte*, WordCount, ChunkedHash, ChunkedEq>(0, ChunkedHash{end}, ChunkedEq{end});
        m_word_counts.reserve(words.size());
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    inline void incrementWordCount(const bpe::Word& word)
    {
        //-----------------------------------------------------------------------------------//
        std::shared_lock<std::shared_mutex> read_lock(m_shared_mutex);

        auto it = m_word_counts.find(word.bytes);
        if(it != m_word_counts.end())
        {
            it->second.increment();
            read_lock.unlock();
            return;
        }

        read_lock.unlock();
        //-----------------------------------------------------------------------------------//

        //-----------------------------------------------------------------------------------//
        std::unique_lock<std::shared_mutex> write_lock(m_shared_mutex);

        it = m_word_counts.find(word.bytes);
        if(it == m_word_counts.end())
        {
            it = m_word_counts.try_emplace(word.bytes).first;
        }
        it->second.increment();

        write_lock.unlock();
        //-----------------------------------------------------------------------------------//
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //