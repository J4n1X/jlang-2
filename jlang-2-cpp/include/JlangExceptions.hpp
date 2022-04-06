#ifndef JLANGEXCEPTIONS_H_
#define JLANGEXCEPTIONS_H_

#include "JlangObjects.hpp"
#include <exception>
#include <sstream>
#include <string>

namespace jlang {
class ParserException : std::exception {
    std::string message;
    const Token token;

  public:
    ParserException(std::string msg, const Token token)
        : message(msg), token(token) {}
    const char *what() const throw() {
        // copy the stringstream into a string
        return message.c_str();
    }
    const Token &which() const throw() { return token; }
    const std::string where() const throw() { return token.location.display(); }
};
} // namespace jlang

#endif