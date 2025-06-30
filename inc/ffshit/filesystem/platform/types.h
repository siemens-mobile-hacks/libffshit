#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_TYPES_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_TYPES_H

#include <unordered_map>
#include <string>

namespace FULLFLASH {

enum class Platform {
    UNK,
    EGOLD_CE,
    SGOLD,
    SGOLD2,
    SGOLD2_ELKA
};

const std::unordered_map<Platform, std::string> PlatformToString = {
    { Platform::EGOLD_CE,     "EGOLD_CE" },
    { Platform::SGOLD,        "SGOLD" },
    { Platform::SGOLD2,       "SGOLD2" },
    { Platform::SGOLD2_ELKA,  "SGOLD2_ELKA" },
};

const std::unordered_map<std::string, Platform> StringToPlatform = {
    { "EGOLD_CE",       Platform::EGOLD_CE },
    { "SGOLD",          Platform::SGOLD },
    { "SGOLD2",         Platform::SGOLD2 },
    { "SGOLD2_ELKA",    Platform::SGOLD2_ELKA },
};

};


#endif
