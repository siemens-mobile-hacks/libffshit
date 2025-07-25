#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_ATTRIBUTES_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_ATTRIBUTES_H

#include <cstdint>

namespace FULLFLASH {
namespace Filesystem {

enum class FileAttributes : uint16_t {
    READONLY     = 0x0001, 
    HIDDEN       = 0x0002, 
    SYSTEM       = 0x0004, 
    DIRECTORY    = 0x0010, 
};

class Attributes {
    public:
        Attributes();
        Attributes(uint32_t value);

        bool is_directory() const;
        bool is_readonly() const;
        bool is_hidden() const;
        bool is_system() const;

    private:
        bool directory  = false;
        bool readonly   = false;
        bool hidden     = false;
        bool system     = false;
};

};
};
#endif
