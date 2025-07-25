#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_DIRECTORY_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_DIRECTORY_H

#include <memory>
#include <string>
#include <vector>

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/structure/file.h"

namespace FULLFLASH {
namespace Filesystem {

class Directory {
    public:
        using Ptr           = std::shared_ptr<Directory>;
        using Directories   = std::vector<Ptr>;

        Directory(const std::string name, const std::string path);
        Directory(const std::string name, const std::string path, const Attributes &attributes);
        Directory(const std::string name, const std::string path, const Attributes &attributes, const TimePoint &timestamp);

        static Ptr              build(const std::string name, const std::string path);
        static Ptr              build(const std::string name, const std::string path, const Attributes &attributes);
        static Ptr              build(const std::string name, const std::string path, const Attributes &attributes, const TimePoint &timestamp);

        void                    add_subdir(Ptr dir);
        void                    add_file(File::Ptr file);

        const std::string &     get_name() const;
        const std::string &     get_path() const;

        const Attributes &      get_attributes() const;
        const TimePoint         get_timestamp() const;

        const Directories &     get_subdirs() const;
        const File::Files &     get_files() const;

    private:
        std::string             name;
        std::string             path;

        Attributes              attributes;
        TimePoint               timestamp;

        Directories             subdirs;
        File::Files             files;
};


};
};

#endif
