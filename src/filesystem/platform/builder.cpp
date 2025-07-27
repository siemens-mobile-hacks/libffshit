#include "ffshit/filesystem/platform/builder.h"

namespace FULLFLASH {
namespace Filesystem {

Filesystem::Base::Ptr build(Platform::Type platform, Partitions::Partitions::Ptr partitions) {
    switch(platform) {
        case Platform::Type::EGOLD_CE:     return FULLFLASH::Filesystem::EGOLD_CE::build(partitions);
        case Platform::Type::SGOLD:        return FULLFLASH::Filesystem::SGOLD::build(partitions);
        case Platform::Type::SGOLD2:       return FULLFLASH::Filesystem::SGOLD2::build(partitions);
        case Platform::Type::SGOLD2_ELKA:  return FULLFLASH::Filesystem::SGOLD2_ELKA::build(partitions);
        case Platform::Type::UNK:
        default: {
            throw FULLFLASH::Exception("Unknown platform");
        }
    };
}

};
};
