#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BUILDER_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BUILDER_H

#include <ffshit/filesystem/platform/platform.h>

namespace FULLFLASH {
namespace Filesystem {

Filesystem::Base::Ptr build(Platform platform, Blocks &blocks);

};
};

#endif
