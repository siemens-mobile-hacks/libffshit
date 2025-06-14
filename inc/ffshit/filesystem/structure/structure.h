#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_STRUCTURE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_STRUCTURE_H

#include "ffshit/filesystem/structure/file.h"
#include "ffshit/filesystem/structure/directory.h"

#include <map>

namespace FULLFLASH {
namespace Filesystem {

using FSMap = std::map<std::string, Directory::Ptr>;

};
};

#endif
