#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H

#include <memory>

namespace FULLFLASH {
namespace Filesystem  {

class Base {
    public:
        using Ptr = std::shared_ptr<Base>;

        Base() { }

        virtual void load() = 0;
        virtual void extract(std::string path, bool overwrite) = 0;

    private:
};

};
};

#endif
