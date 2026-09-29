#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H

#include <memory>
#include <fmt/format.h>

#include "ffshit/filesystem/structure/structure.h"
#include "ffshit/partition/partitions.h"
#include "ffshit/platform/types.h"

namespace FULLFLASH {
namespace Filesystem  {

namespace Write {
    class Session;
};

class Base {
    public:
        using Ptr = std::shared_ptr<Base>;

        Base() :    verbose_processing(false),
                    verbose_headers(false),
                    verbose_data(false)
                    { }

        void                            log_verbose_processing(bool enabled)    { this->verbose_processing  = enabled; }
        void                            log_verbose_headers(bool enabled)       { this->verbose_headers     = enabled; }
        void                            log_verbose_data(bool enabled)          { this->verbose_data        = enabled; }

        virtual void                    load(bool skip_broken = false, bool skip_dup = false, std::vector<std::string> parts_to_extract = {}) = 0;
        virtual const Directory::Ptr    get_root() const = 0;

        // Paths start with the partition name, e.g. "FFS_0/Misc/photo.jpg". The changes go to the
        // fullflash in memory, FULLFLASH::save() writes it out. An operation that throws leaves the
        // fullflash unchanged.

        // Creates the file, or replaces the file of that name. The parent directory must exist.
        virtual void                    write_file(const std::string &path, const RawData &data, const TimePoint &timestamp);
        // The parent directory must exist
        virtual void                    create_directory(const std::string &path, const TimePoint &timestamp);
        // Removes a file or an empty directory
        virtual void                    remove(const std::string &path);

        virtual ~Base() { }

    protected:
        bool verbose_processing;
        bool verbose_headers;
        bool verbose_data;

        // Built on the first write
        Write::Session &                write_session(Platform::Type platform, Partitions::Partitions::Ptr partitions, Directory::Ptr root);

    private:
        std::shared_ptr<Write::Session> session;
};

static const std::string ROOT_NAME = "FFS";
static const std::string ROOT_PATH = fmt::format("/{}/", ROOT_NAME);

};
};

#endif
