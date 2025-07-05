#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H

#include <memory>

#include "ffshit/filesystem/structure/structure.h"

namespace FULLFLASH {
namespace Filesystem  {

class Base {
    public:
        using Ptr = std::shared_ptr<Base>;

        Base() { }

        virtual void            load(bool skip_broken = false, bool skip_dup = false) = 0;
        virtual const FSMap &   get_filesystem_map() const = 0;

    private:
};

};
};

#endif
