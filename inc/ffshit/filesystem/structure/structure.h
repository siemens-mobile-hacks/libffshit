#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_STRUCTURE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_STRUCTURE_H

#include "ffshit/filesystem/structure/file.h"
#include "ffshit/filesystem/structure/directory.h"

#include "thirdparty/ordered_map.h"

namespace FULLFLASH {
namespace Filesystem {

using FSMap = tsl::ordered_map<std::string, Directory::Ptr>;

};
};

#endif
