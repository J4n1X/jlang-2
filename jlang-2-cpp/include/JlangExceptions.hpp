#ifndef JLANGEXCEPTIONS_H_
#define JLANGEXCEPTIONS_H_

#include <exception>
#include <string>
#include <sstream>
#include "JlangObjects.hpp"

namespace jlang {
    class ParserException : std::exception {
        const char *message;
        const Token token;
        public:
        ParserException(const char* msg, const Token token) : message(msg), token(token) {}
        const char *what() const throw() {
            // copy the stringstream into a string
            return message;
        }
        const Token &which() const throw() {
            return token;
        }
        const std::string where() const throw() {
            return token.location.display();
        }
    };
}

#endif