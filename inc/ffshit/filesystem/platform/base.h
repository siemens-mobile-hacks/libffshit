#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H

#include <memory>
#include <fmt/format.h>

#include "ffshit/filesystem/structure/structure.h"


namespace FULLFLASH {
namespace Filesystem  {

class Base {
    public:
        using Ptr = std::shared_ptr<Base>;

        Base() { }

        virtual void                    load(bool skip_broken = false, bool skip_dup = false) = 0;
        virtual const Directory::Ptr    get_root() const = 0;

    private:
};

static const std::string ROOT_NAME = "FFS";
static const std::string ROOT_PATH = fmt::format("/{}/", ROOT_NAME);

};
};

#endif
