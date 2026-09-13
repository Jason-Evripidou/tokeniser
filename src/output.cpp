// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Dependencies
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
// Internal header files.
#include "bpe.h"

// Standard library.
#include <fstream>
#include <iostream>
#include <stdexcept>
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //


// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //
namespace bpe
{
    //---------------------------------------------------------------------------------------//
    /*
    -   using Byte = unsigned char;
    -   struct TokenCount
    |   {
    |       std::vector<Byte> token;
    |       std::size_t count = 0;
    |   };
    -   struct Results
    |   {
    |       std::vector<WordCount> word_counts;
    |       std::vector<CharSplit> char_splits;
    |       std::vector<TokenCount> tokens;
    |   };
    */
    //---------------------------------------------------------------------------------------//
    void write_output(const Results& results, const std::string& path)
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw std::runtime_error("cannot write file: " + path);
        }
        for (const TokenCount& token : results.tokens)
        {
            out.write(reinterpret_cast<const char*>(token.token.data()),
                    static_cast<std::streamsize>(token.token.size()));
            out << ' ' << token.count << '\n';
        }
        if (!out)
        {
            throw std::runtime_error("failed while writing file: " + path);
        }
    }
    //---------------------------------------------------------------------------------------//

    //---------------------------------------------------------------------------------------//
    /*
    -   using Byte = unsigned char;
    -   struct WordCount
    |   {
    |       std::vector<Byte> word;
    |       std::size_t count = 0;
    |   };
    -   struct CharSplit
    |   {
    |       std::vector<Byte> chars;
    |       std::size_t count = 0;
    |   };
    -   struct Results
    |   {
    |       std::vector<WordCount> word_counts;
    |       std::vector<CharSplit> char_splits;
    |       std::vector<TokenCount> tokens;
    |   };
    */
    //---------------------------------------------------------------------------------------//
    void print_task1(const Results& results)
    {
        for (const WordCount& entry : results.word_counts)
        {
            std::cout.write(reinterpret_cast<const char*>(entry.word.data()),
                            static_cast<std::streamsize>(entry.word.size()));
            std::cout << ' ' << entry.count << '\n';
        }

        for (const CharSplit& entry : results.char_splits)
        {
            for (std::size_t i = 0; i < entry.chars.size(); ++i)
            {
                if (i > 0)
                {
                    std::cout << ' ';
                }
                std::cout << static_cast<unsigned char>(entry.chars[i]);
            }
            std::cout << ' ' << entry.count << '\n';
        }
    }
    //---------------------------------------------------------------------------------------//
}
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### //