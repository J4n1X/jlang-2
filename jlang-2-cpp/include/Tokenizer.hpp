#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <string>
#include <vector>

#include "JlangObjects.hpp"

namespace jlang {

    class Tokenizer {
        private:
            std::string in_file_path;
            std::vector<std::string> in_file_lines;
            std::vector<Token> tokens;
        public:
            Tokenizer(std::string in_file_path);
            bool tokenize();
            std::vector<Token> &get_tokens() {
                return tokens;
            }
    };

}
#endif