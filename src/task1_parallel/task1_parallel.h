// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
/*
-   For Task 1 we do the following:

    -   Sequential (Done for us):
        -   std::vector<Byte> read_file(const std::string& path);
        -   Take input file (ASCII text file) and store the data in a std::vector<Byte>. Let us
        |   define this output as "_input_".
            -   _input_ must remain in memory for the whole task 1 lifetime. Is destroyed after
            |   Task 1 completes.

        -   std::vector<Word> split_words(std::vector<Byte>& input);
        -   Create _split_words_ using _input_.

    - Parallel part 1 (Task 1.1): Word Frequency Counting
    - Parallel part 2 (Task 1.2): Character Splitting
*/
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#ifndef TASK1_PARALLEL_H
#define TASK1_PARALLEL_H
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "../bpe.h"

// Standard library.
#include <vector>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
struct WordFreqs
{

};
void wordFrequencyCounting(const std::vector<bpe::Word>& words)
{

}
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
#endif
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //