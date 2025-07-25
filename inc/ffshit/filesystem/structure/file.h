#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_FILE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_STRUCTURE_FILE_H

#include <memory>
#include <string>
#include <vector>

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/help.h"
#include "ffshit/filesystem/structure/attributes.h"

namespace FULLFLASH {
namespace Filesystem {

class File {
    public:
        using Ptr   = std::shared_ptr<File>;
        using Files = std::vector<Ptr>;

        File(const std::string name, const std::string path, const RawData &data, const Attributes &attributes);
        File(const std::string name, const std::string path, const RawData &data, const Attributes &attributes, const TimePoint &timestamp);

        static Ptr          build(const std::string name, const std::string path, const RawData &data, const Attributes &attributes);
        static Ptr          build(const std::string name, const std::string path, const RawData &data, const Attributes &attributes, const TimePoint &timestamp);

        const std::string & get_name() const;
        const std::string & get_path() const;

        const Attributes &  get_attributes() const;
        const TimePoint     get_timestamp() const;

        const RawData &     get_data() const;
        const size_t        get_size() const;

    private:
        std::string         name;
        std::string         path;

        Attributes          attributes;
        TimePoint           timestamp;

        RawData             data;
};


};
};

#endif
