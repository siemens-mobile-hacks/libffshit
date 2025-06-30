#include "ffshit/filesystem/platform/builder.h"

namespace FULLFLASH {
namespace Filesystem {

Filesystem::Base::Ptr build(Platform platform, Partitions::Partitions::Ptr partitions) {
    switch(platform) {
        case FULLFLASH::Platform::EGOLD_CE:     return FULLFLASH::Filesystem::EGOLD_CE::build(partitions);
        case FULLFLASH::Platform::SGOLD:        return FULLFLASH::Filesystem::SGOLD::build(partitions);
        case FULLFLASH::Platform::SGOLD2:       return FULLFLASH::Filesystem::SGOLD2::build(partitions);
        case FULLFLASH::Platform::SGOLD2_ELKA:  return FULLFLASH::Filesystem::SGOLD2_ELKA::build(partitions);
        case FULLFLASH::Platform::UNK:
        default: {
            throw FULLFLASH::Exception("Unknown platform");
        }
    };
}

};
};
