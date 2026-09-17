// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Dependencies
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "bpe.h"
#include "absl/log/log.h"
#include "task1/count_system/count_system.h"
#include "task1/merge_sort_system/merge_sort_system.h"

// Standard library.
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <utility>

#include <omp.h>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
namespace bpe
{
    namespace
    {
        std::int64_t elapsed_ms
        (
            const std::chrono::steady_clock::time_point& start,
            const std::chrono::steady_clock::time_point& end
        )
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        }

        constexpr std::uint64_t kOnes = ~0ULL / 255;
        std::uint64_t hasless_any(std::uint64_t x, unsigned limit)
        {
            return (x - kOnes * limit) & ~x & (kOnes * 128);
        }

        std::uint64_t haszero(std::uint64_t x) { return hasless_any(x, 1); }

        struct ByteStrLess
        {
            bool operator()(const Byte* a, const Byte* b) const
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
            const Byte* end;
            std::size_t operator()(const Byte* s) const
            {
                std::size_t h = 1469598103934665603ULL;
                const Byte* p = s;
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

                const Byte* q = p;
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
            const Byte* end;
            bool operator()(const Byte* a, const Byte* b) const
            {
                if (a == b)
                {
                    return true;
                }
                const Byte* pa = a;
                const Byte* pb = b;
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
    }

    // task1: count distinct words; a SWAR (SIMD Within A Register) trick finds each
    // word's end 8 bytes at a time.
    /*
    -   Original task1.
    void task1(const std::vector<Word>& words, Results& results)
    {
        results.word_counts.clear();
        results.char_splits.clear();

        const std::chrono::steady_clock::time_point t_wc0 = std::chrono::steady_clock::now();

        if (words.empty())
        {
            LOG(INFO) << "word count: 0 ms; char split: 0 ms";
            return;
        }

        const Byte* end = words.back().bytes;
        while (*end)
        {
            ++end;
        }
        ++end;

        std::unordered_map<const Byte*, std::size_t, ChunkedHash, ChunkedEq> counts(0, ChunkedHash{end}, ChunkedEq{end});
        counts.reserve(words.size());
        for (const Word& word : words)
        {
            ++counts[word.bytes];
        }

        std::vector<std::pair<const Byte*, std::size_t>> sorted;
        sorted.reserve(counts.size());
        for (const auto& entry : counts)
        {
            sorted.emplace_back(entry.first, entry.second);
        }
        std::sort
        (
            sorted.begin(),
            sorted.end(),
            [](const std::pair<const Byte*, std::size_t>& a, const std::pair<const Byte*, std::size_t>& b)
            {
                return ByteStrLess{}(a.first, b.first);
            }
        );

        const std::chrono::steady_clock::time_point t_wc1 = std::chrono::steady_clock::now();

        results.word_counts.reserve(sorted.size());
        results.char_splits.reserve(sorted.size());
        for (const auto& entry : sorted)
        {
            const Byte* s = entry.first;
            const std::size_t n = std::strlen(reinterpret_cast<const char*>(s));
            const std::vector<Byte> bytes(s, s + n);
            results.word_counts.push_back(WordCount{bytes, entry.second});
            results.char_splits.push_back(CharSplit{bytes, entry.second});
        }

        const std::chrono::steady_clock::time_point t_cs1 = std::chrono::steady_clock::now();

        LOG(INFO) << "word count: " << elapsed_ms(t_wc0, t_wc1) << " ms";
        LOG(INFO) << "char split: " << elapsed_ms(t_wc1, t_cs1) << " ms";
    }
    */

    void task1(const std::vector<bpe::Word>& words, bpe::Results& results)
    {
        results.word_counts.clear();
        results.char_splits.clear();

        std::size_t words_size = words.size();
        if(words_size == static_cast<std::size_t>(0))
        {
            LOG(INFO) << "word count: 0 ms; char split: 0 ms";
            return;
        }

        const Byte* end = words.back().bytes;
        while (*end)
        {
            ++end;
        }
        ++end;

        //-----------------------------------------------------------------------------------//
        // Superceeded.
        //-----------------------------------------------------------------------------------//
        std::size_t num_count_system_workers = static_cast<std::size_t>(10);
        std::size_t chunk_size = words_size / num_count_system_workers;
        if(chunk_size < static_cast<std::size_t>(1))
        {
            chunk_size = static_cast<std::size_t>(1);
        }
        CountSystem count_system(words, chunk_size, num_count_system_workers);
        //-----------------------------------------------------------------------------------//

        MergeSortSystem merge_sort_system;

        const std::chrono::steady_clock::time_point t_wc0 = std::chrono::steady_clock::now();

        //-----------------------------------------------------------------------------------//
        // Task 1.1.1: Word frequency counting.
        //-----------------------------------------------------------------------------------//
        //count_system.countWords();
        //count_system.m_word_counts.printWordCounts(std::string("out_1_1_1_word_counts.txt"));

        const int num_threads = omp_get_max_threads();
        std::vector<std::unordered_map<const Byte*, std::size_t, ChunkedHash, ChunkedEq>> thread_counts;
        thread_counts.reserve(num_threads);

        for(int i = 0; i < num_threads; i++)
        {
            thread_counts.emplace_back(0, ChunkedHash{end}, ChunkedEq{end});
        }

        #pragma omp parallel
        {
            const int thread_id = omp_get_thread_num();

            auto& local_counts = thread_counts[thread_id];

            local_counts.reserve(words.size() / num_threads + 1);

            #pragma omp for
            for(std::size_t i = 0; i < words.size(); i++)
            {
                ++local_counts[words[i].bytes];
            }
        }

        std::unordered_map<const Byte*, std::size_t, ChunkedHash, ChunkedEq> counts(0, ChunkedHash{end}, ChunkedEq{end});
        counts.reserve(words.size());

        for(const auto& local_counts : thread_counts)
        {
            for(const auto& entry : local_counts)
            {
                counts[entry.first] += entry.second;
            }
        }
        //-----------------------------------------------------------------------------------//

        //-----------------------------------------------------------------------------------//
        // Task 1.1.2: Sort Words.
        //-----------------------------------------------------------------------------------//
        std::vector<std::pair<const bpe::Byte*, std::size_t>> sorted;
        sorted.reserve(counts.size());
        for (const auto& entry : counts)
        {
            sorted.emplace_back(entry.first, entry.second);
        }
        merge_sort_system.parallelMergeSort(sorted);
        //merge_sort_system.printData(std::string("out_1_1_2_sorted_words.txt"), sorted);
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