#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_FILE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_FILE_H

#include <memory>
#include <string>
#include <vector>

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/help.h"

namespace FULLFLASH {
namespace Filesystem {

class File {
    public:
        using Ptr   = std::shared_ptr<File>;
        using Files = std::vector<Ptr>;

        File(const std::string name, const std::string path, const RawData &data);
        File(const std::string name, const std::string path, const TimePoint &timestamp, const RawData &data);

        static Ptr          build(const std::string name, const std::string path, const RawData &data);
        static Ptr          build(const std::string name, const std::string path, const TimePoint &timestamp, const RawData &data);

        const std::string & get_name() const;
        const std::string & get_path() const;
        const TimePoint     get_timestamp() const;

        const RawData &     get_data() const;
        const size_t        get_size() const;

    private:
        std::string         name;
        std::string         path;
        TimePoint           timestamp;

        RawData             data;
};


};
};

#endif
