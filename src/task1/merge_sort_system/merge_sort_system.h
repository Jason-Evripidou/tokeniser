// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef MERGE_SORT_SYSTEM_H
#define MERGE_SORT_SYSTEM_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../../bpe.h"

#include <omp.h>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct MergeSortSystem
{
private:
    //---------------------------------------------------------------------------------------//
    /*
    -   Following code copied from task1.cpp:
        -   struct ByteStrLess;
    */
    //---------------------------------------------------------------------------------------//
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
    //---------------------------------------------------------------------------------------//

public:
    //---------------------------------------------------------------------------------------//
    inline void merge
    (
        std::vector<std::pair<const bpe::Byte*, std::size_t>>& data  ,
        std::size_t                                            left  ,
        std::size_t                                            middle,
        std::size_t                                            right ,
        std::vector<std::pair<const bpe::Byte*, std::size_t>>& temp 
    )
    {
        std::size_t left_index  = left;
        std::size_t right_index = middle + 1;
        std::size_t temp_index  = left;

        while( (left_index <= middle) && (right_index <= right) )
        {
            if(ByteStrLess{}(data[right_index].first, data[left_index].first))
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
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    inline void mergeSortHelper
    (
        std::vector<std::pair<const bpe::Byte*, std::size_t>>& data ,
        std::size_t                                            left ,
        std::size_t                                            right,
        std::vector<std::pair<const bpe::Byte*, std::size_t>>& temp 
    )
    {
        if(left >= right) { return; }

        std::size_t middle = ((right - left) / 2) + left;

        this->mergeSortHelper(data, left, middle, temp);
        this->mergeSortHelper(data, middle + 1, right, temp);

        this->merge(data, left, middle, right, temp);
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    inline void mergeSort(std::vector<std::pair<const bpe::Byte*, std::size_t>>& data)
    {
        if(data.size() <= 1) { return; }

        std::vector<std::pair<const bpe::Byte*, std::size_t>> temp;
        temp.resize(data.size());

        this->mergeSortHelper(data, 0, data.size() - 1, temp);
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    inline void parallelMergeSort(std::vector<std::pair<const bpe::Byte*, std::size_t>>& data)
    {
        if(data.size() <= 1) { return; }

        std::vector<std::pair<const bpe::Byte*, std::size_t>> temp;
        temp.resize(data.size());

        const int num_threads = omp_get_max_threads();

        const std::size_t chunk_size = (data.size() + num_threads - 1) / num_threads;

        //-----------------------------------------------------------------------------------//
        #pragma omp parallel for
        for(int thread_index = 0; thread_index < num_threads; thread_index++)
        {
            const std::size_t left = static_cast<std::size_t>(thread_index) * chunk_size;

            if(left >= data.size()) { continue; }

            std::size_t right = std::min(left + chunk_size, data.size()) - 1;
            this->mergeSortHelper(data, left, right, temp);
        }
        //-----------------------------------------------------------------------------------//

        std::size_t current_chunk_size = chunk_size;

        while(current_chunk_size < data.size())
        {
            std::size_t merge_size = current_chunk_size * 2;

            for(std::size_t left = 0; left < data.size(); left += merge_size)
            {
                std::size_t middle = left + current_chunk_size - 1;

                if(middle >= data.size()) { break; }

                std::size_t right = std::min(left + merge_size - 1, data.size() - 1);

                this->merge(data, left, middle, right, temp);
            }

            current_chunk_size *= 2;
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    // Debugging.
    //---------------------------------------------------------------------------------------//
    // This function was generated by AI.
    inline void printData
    (
        const std::string& output_filename,
        std::vector<std::pair<const bpe::Byte*, std::size_t>>& data
    ) const
    {
        std::ofstream output_file(output_filename);

        if(!output_file.is_open())
        {
            return;
        }

        for(const auto& entry : data)
        {
            const bpe::Byte* bytes = entry.first;
            const std::size_t count = entry.second;

            output_file << "Word: ";
            std::size_t i = 0;

            while(bytes[i] != 0)
            {
                output_file << static_cast<char>(bytes[i]); ++i;
            }

            output_file << " | Count: " << count << '\n';
        }
    }
    //---------------------------------------------------------------------------------------//
};
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //