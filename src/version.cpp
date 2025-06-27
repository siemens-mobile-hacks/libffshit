#include "ffshit/version.h"

namespace FULLFLASH {

std::string get_libffshit_version() {
    return std::string(DEF_VERSION_STRING);
}

std::string get_libffshit_version_major() {
    return std::string(DEF_VERSION_STRING_MAJOR);
}

std::string get_libffshit_version_minor() {
    return std::string(DEF_VERSION_STRING_MINOR);
}

std::string get_libffshit_version_patch() {
    return std::string(DEF_VERSION_STRING_PATCH);
}

};
