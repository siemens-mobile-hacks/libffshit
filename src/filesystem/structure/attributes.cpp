#include "ffshit/filesystem/structure/attributes.h"

namespace FULLFLASH {
namespace Filesystem {

Attributes::Attributes() :
    directory(false),
    system(false),
    hidden(false),
    readonly(false) {
}

Attributes::Attributes(uint32_t value) {
    directory   = value & static_cast<uint16_t>(FileAttributes::DIRECTORY);
    system      = value & static_cast<uint16_t>(FileAttributes::SYSTEM);
    hidden      = value & static_cast<uint16_t>(FileAttributes::HIDDEN);
    readonly    = value & static_cast<uint16_t>(FileAttributes::READONLY);
}

bool Attributes::is_directory() const {
    return directory;
}

bool Attributes::is_readonly() const {
    return readonly;
}

bool Attributes::is_hidden() const {
    return hidden;
}

bool Attributes::is_system() const {
    return system;
}

};
};
