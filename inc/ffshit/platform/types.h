#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_TYPES_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_TYPES_H

#include <unordered_map>
#include <string>

namespace FULLFLASH {
namespace Platform {

enum class Type {
    UNK,
    EGOLD_CE,
    SGOLD,
    SGOLD2,
    SGOLD2_ELKA
};

const std::unordered_map<Type, std::string> TypeToString = {
    { Type::EGOLD_CE,     "EGOLD_CE" },
    { Type::SGOLD,        "SGOLD" },
    { Type::SGOLD2,       "SGOLD2" },
    { Type::SGOLD2_ELKA,  "SGOLD2_ELKA" },
};

const std::unordered_map<std::string, Type> StringToType = {
    { "EGOLD_CE",       Type::EGOLD_CE },
    { "SGOLD",          Type::SGOLD },
    { "SGOLD2",         Type::SGOLD2 },
    { "SGOLD2_ELKA",    Type::SGOLD2_ELKA },
};

};
};

#endif
