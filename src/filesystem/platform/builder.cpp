#include "ffshit/filesystem/platform/builder.h"

namespace FULLFLASH {
namespace Filesystem {

Filesystem::Base::Ptr build(Platform platform, Blocks &blocks) {
    switch(platform) {
        case FULLFLASH::Platform::X65: return FULLFLASH::Filesystem::SGOLD::build(blocks);
        case FULLFLASH::Platform::X75: return FULLFLASH::Filesystem::NewSGOLD::build(blocks);
        case FULLFLASH::Platform::X85: return FULLFLASH::Filesystem::NewSGOLD_X85::build(blocks);
        default:
        case FULLFLASH::Platform::UNK: {
            throw FULLFLASH::Exception("Unknown platform");
        }
    };
}

};
};
