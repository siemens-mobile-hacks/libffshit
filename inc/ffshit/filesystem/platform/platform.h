#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_PLATFORM_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_PLATFORM_H

#include "ffshit/filesystem/platform/sgold.h"
#include "ffshit/filesystem/platform/newsgold.h"
#include "ffshit/filesystem/platform/newsgold_x85.h"
#include "ffshit//filesystem/platform/types.h"
#include "ffshit/ex.h"

namespace FULLFLASH {
namespace Filesystem {

Filesystem::Base::Ptr build(Platform platform, Blocks &blocks) {
    switch(platform) {
        case FULLFLASH::Platform::X65: return FULLFLASH::Filesystem::SGOLD::build(blocks);
        case FULLFLASH::Platform::X75: return FULLFLASH::Filesystem::NewSGOLD::build(blocks);
        case FULLFLASH::Platform::X85: return FULLFLASH::Filesystem::NewSGOLD_X85::build(blocks);
        case FULLFLASH::Platform::UNK: {
            throw FULLFLASH::Exception("Unknown platform");
        }
    };
}

};
};

#endif
