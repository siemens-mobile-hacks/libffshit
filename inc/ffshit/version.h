#ifndef LIBFFSHIT_FULLFLASH_VERSION_H
#define LIBFFSHIT_FULLFLASH_VERSION_H

#include <string>

#ifndef DEF_VERSION_STRING
    #define DEF_VERSION_STRING "unknown"
#endif

#ifndef DEF_VERSION_STRING_MAJOR
    #define DEF_VERSION_STRING_MAJOR ""
#endif

#ifndef DEF_VERSION_STRING_MINOR
    #define DEF_VERSION_STRING_MINOR ""
#endif

#ifndef DEF_VERSION_STRING_PATCH
    #define DEF_VERSION_STRING_PATCH ""
#endif

namespace FULLFLASH {

std::string get_libffshit_version();
std::string get_libffshit_version_major();
std::string get_libffshit_version_minor();
std::string get_libffshit_version_patch();

};

#endif
