#include <fstream>
#include <iostream>
#include <string>
#include <vector>

//#define JLANGOBJECTS_IMPLEMENTATION
#include "JlangObjects.hpp"
#include "Tokenizer.hpp"

using namespace jlang;

// damn, this is elegant
std::string read_file(std::string file_path) {
    std::ifstream file(file_path);
    std::string contents((std::istreambuf_iterator<char>(file)),
                         (std::istreambuf_iterator<char>()));
    return contents;
}

std::vector<std::string> read_lines(std::string file_path) {
    std::vector<std::string> lines;
    std::ifstream file(file_path);
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string get_word(std::string::iterator it, std::string::iterator end) {
    std::string word;
    while (it != end && (isalnum(*it) || *it == '_' || *it == '-')) {
        word += *it;
        it++;
    }
    return word;
}

typedef struct {
    std::string value;
    bool success;
} get_string_result;

get_string_result get_string(std::string::iterator it,
                             std::string::iterator end) {
    std::string string;
    it++;
    while (it != end && *it != '"') {
        string += *it;
        it++;
    }
    if (*it != '"') {
        std::cout << "Error: Expected '\"' at " << it - end << '\n';
        return {"", false};
    }
    return {string, true};
}

std::string to_upper(std::string str) {
    std::string upper;
    for (auto c : str) {
        upper += toupper(c);
    }
    return upper;
}

std::string get_int_text(std::string::iterator it, std::string::iterator end) {
    std::string word = "";
    while (it != end && isdigit(*it)) {
        word += *it;
        it++;
    }
    return word;
}

Tokenizer::Tokenizer(std::string in_file_path) {
    this->in_file_path = in_file_path;
    this->in_file_lines = read_lines(in_file_path);
    if (!tokenize())
        this->tokens.clear();
}

bool Tokenizer::tokenize() {
    std::string word = "";
    std::size_t column_pos = 1;
    std::size_t line_pos = 1;

    for (auto &line : this->in_file_lines) {
        // this is an old way to do it, but I don't want to spend too much time
        // on a more elegant way
        bool newline = false;
        std::size_t char_pos = 0;
        while (line.length() > char_pos && !newline) {
            while (isspace(line[char_pos]))
                if (char_pos + 1 >= line.length()) {
                    break;
                } else {
                    char_pos++;
                }

            column_pos = char_pos + 1;

            word = "";
            if (isalpha(line[char_pos])) {
                word = get_word(line.begin() + char_pos, line.end());
                char_pos += word.length();
                auto upper_word = to_upper(word);
                // to upper case
                if (get_keyword_by_name(upper_word.c_str()) !=
                    Keyword::INVALID) {
                    auto keyword = get_keyword_by_name(upper_word.c_str());
                    this->tokens.push_back(Token(
                        TokenType::KEYWORD, word,
                        Location(this->in_file_path, line_pos, column_pos),
                        TokenValue(keyword)));
                } else if (get_operator_by_name(upper_word.c_str()) !=
                           Operator::INVALID) {
                    auto operator_ = get_operator_by_name(upper_word.c_str());
                    this->tokens.push_back(Token(
                        TokenType::OPERATOR, word,
                        Location(this->in_file_path, line_pos, column_pos),
                        TokenValue(operator_)));
                } else if (get_intrinsic_by_name(upper_word.c_str()) !=
                           Intrinsic::INVALID) {
                    auto intrinsic = get_intrinsic_by_name(upper_word.c_str());
                    this->tokens.push_back(Token(
                        TokenType::INTRINSIC, word,
                        Location(this->in_file_path, line_pos, column_pos),
                        TokenValue(intrinsic)));
                } else if (get_expr_type_by_name(upper_word.c_str()) !=
                           ExprType::INVALID) {
                    auto type = get_expr_type_by_name(upper_word.c_str());
                    this->tokens.push_back(Token(
                        TokenType::TYPE, word,
                        Location(this->in_file_path, line_pos, column_pos),
                        TokenValue(type)));
                } else {
                    // if word contains -, throw an error
                    auto err_pos = word.find('-');
                    if (err_pos != std::string::npos) {
                        std::cout << "Invalid character found at "
                                  << Location(this->in_file_path, line_pos,
                                              column_pos + err_pos)
                                         .display()
                                  << '\n';
                        return false;
                    } else {
                        this->tokens.push_back(Token(
                            TokenType::IDENTIFIER, word,
                            Location(this->in_file_path, line_pos, column_pos),
                            word));
                    }
                }
            } else if (isdigit(line[char_pos])) {
                word = get_int_text(line.begin() + char_pos, line.end());
                char_pos += word.length();
                this->tokens.push_back(
                    Token(TokenType::INT_LITERAL, word,
                          Location(this->in_file_path, line_pos, column_pos),
                          TokenValue(std::stoull(word))));
            } else if (line[char_pos] == '"') {
                auto string_value =
                    get_string(line.begin() + char_pos, line.end());
                char_pos += string_value.value.length() + 2;
                if (string_value.success) {
                    this->tokens.push_back(Token(
                        TokenType::STRING_LITERAL, string_value.value,
                        Location(this->in_file_path, line_pos, column_pos),
                        TokenValue(string_value.value)));
                } else {
                    return false;
                }
            } else if (line[char_pos] == '(') {
                char_pos++;
                this->tokens.push_back(
                    Token(TokenType::PAREN_BLOCK_START, "(",
                          Location(this->in_file_path, line_pos, column_pos),
                          TokenValue("(")));
            } else if (line[char_pos] == ')') {
                char_pos++;
                this->tokens.push_back(
                    Token(TokenType::PAREN_BLOCK_END, ")",
                          Location(this->in_file_path, line_pos, column_pos),
                          TokenValue(")")));
            } else if (line[char_pos] == ',') {
                char_pos++;
                this->tokens.push_back(
                    Token(TokenType::ARG_DELIMITER, ",",
                          Location(this->in_file_path, line_pos, column_pos),
                          TokenValue(",")));
            } else if (line[char_pos] == ';') {
                char_pos++;
                break;
            } else {
                std::cout << "Unexpected Symbol: " << line[char_pos] << '\n';
                return false;
            }
        }
        line_pos++;
        column_pos = 1;
    }
    return true;
}