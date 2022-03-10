#ifndef JLANGEXCEPTIONS_H_
#define JLANGEXCEPTIONS_H_

#include <exception>
#include <string>
#include <sstream>
#include "JlangObjects.hpp"

namespace jlang {
    class ParserException : std::exception {
        const char *message;
        const Location location;
        public:
        ParserException(const char* msg, const Location& location) : message(msg), location(location) {}
        const char *what() const throw() {
            // copy the stringstream into a string
            return message;
        }
        const std::string where() const throw() {
            return location.display();
        }
    };
}

#endif