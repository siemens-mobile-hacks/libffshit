#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_WRITE_SESSION_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_WRITE_SESSION_H

#include "filesystem/write/writer.h"

#include "ffshit/filesystem/structure/structure.h"

#include <functional>
#include <map>

namespace FULLFLASH {
namespace Filesystem {
namespace Write {

// The writes to a filesystem: every operation either happens entirely or not at all, and shows in
// the loaded directory tree and in the partitions' block data afterwards
class Session {
    public:
        Session(Platform::Type platform, Partitions::Partitions::Ptr partitions, Directory::Ptr root);

        // The paths start with the partition's name
        void                                write_file(const std::string &path, const RawData &data, const TimePoint &timestamp);
        void                                create_directory(const std::string &path, const TimePoint &timestamp);
        void                                remove(const std::string &path);

    private:
        struct Path {
            std::string                     partition;
            // below the partition's root, at least one
            std::vector<std::string>        names;
        };

        Platform::Type                      platform;
        Partitions::Partitions::Ptr         partitions;
        Directory::Ptr                      root;
        std::map<std::string, Writer::Ptr>  writers;

        Path                                parse(const std::string &path) const;
        Writer &                            writer(const std::string &partition);
        void                                run(const Path &path, const std::function<void(Writer &)> &operation);

        // The parent directory in the loaded tree, when that is loaded, and the path the readers
        // give its files and directories
        Directory::Ptr                      loaded_parent(const Writer &writer, const Path &path, std::string &entries_path) const;
};

};
};
};

#endif
