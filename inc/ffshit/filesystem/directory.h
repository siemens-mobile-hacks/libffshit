#ifndef FULLFLASH_FILESYSTEM_DIRECTORY_H
#define FULLFLASH_FILESYSTEM_DIRECTORY_H

#include <memory>
#include <string>
#include <vector>

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/file.h"

namespace FULLFLASH {
namespace Filesystem {

class Directory {
    public:
        using Ptr           = std::shared_ptr<Directory>;
        using Directories   = std::vector<Ptr>;

        Directory(std::string name);

        static Ptr              build(std::string name);
        void                    add_subdir(Ptr dir);
        void                    add_file(File::Ptr file);
        const std::string       get_name() const;
        const Directories &     get_subdirs() const;
        const File::Files &     get_files() const;

    private:
        std::string             name;
        Directories             subdirs;
        File::Files             files;
};


};
};

#endif
