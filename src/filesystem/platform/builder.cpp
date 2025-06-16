#include "ffshit/filesystem/platform/builder.h"

namespace FULLFLASH {
namespace Filesystem {

Filesystem::Base::Ptr build(Platform platform, Partitions::Partitions::Ptr partitions) {
    switch(platform) {
        case FULLFLASH::Platform::X65: return FULLFLASH::Filesystem::SGOLD::build(partitions);
        case FULLFLASH::Platform::X75: return FULLFLASH::Filesystem::NewSGOLD::build(partitions);
        case FULLFLASH::Platform::X85: return FULLFLASH::Filesystem::NewSGOLD_X85::build(partitions);
        case FULLFLASH::Platform::UNK:
        default: {
            throw FULLFLASH::Exception("Unknown platform");
        }
    };
}

};
};
