#ifndef STRHASH_H_
#define STRHASH_H_
#include <cstddef>
#include <string>

namespace strhash{
    // static unsigned constexpr hash(std::string const& str) {
    //     unsigned h = 0;
    //     for (auto c : str) {
    //         h = h * 131 + c;
    //     }
    //     return h;
    // }

    static unsigned constexpr hash(char const *input) {
        return *input ?
            static_cast<unsigned int>(*input) + 33 * hash(input + 1) :
            5381;
    }
}


#endif