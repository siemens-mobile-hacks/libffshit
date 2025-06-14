#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_TYPES_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_TYPES_H

#include <unordered_map>
#include <string>

namespace FULLFLASH {

enum class Platform {
    UNK,
    X65,
    X75,
    X85
};

const std::unordered_map<Platform, std::string> PlatformToString = {
    { Platform::X65, "X65" },
    { Platform::X75, "X75" },
    { Platform::X85, "X85" },
};

const std::unordered_map<std::string, Platform> StringToPlatform = {
    { "X65", Platform::X65 },
    { "X75", Platform::X75 },
    { "X85", Platform::X85 },
};

};


#endif
