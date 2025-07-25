#include "ffshit/filesystem/structure/file.h"

namespace FULLFLASH {
namespace Filesystem {

File::File(const std::string name, const std::string path, const RawData &data, const Attributes &attributes) : 
    name(name),
    path(path),
    data(data),
    attributes(attributes),
    timestamp(std::chrono::seconds(0)) {
}

File::File(const std::string name, const std::string path, const RawData &data, const Attributes &attributes, const TimePoint &timestamp) : 
    name(name),
    path(path),
    data(data),
    attributes(attributes),
    timestamp(timestamp) {
}

File::Ptr File::build(const std::string name, const std::string path, const RawData &data, const Attributes &attributes) {
    return std::make_shared<File>(name, path, data, attributes);
}

File::Ptr File::build(const std::string name, const std::string path, const RawData &data, const Attributes &attributes, const TimePoint &timestamp) {
    return std::make_shared<File>(name, path, data, attributes, timestamp);
}

const std::string &File::get_name() const {
    return name;
}

const std::string &File::get_path() const {
    return path;
}

const Attributes &File::get_attributes() const {
    return attributes;
}

const TimePoint File::get_timestamp() const {
    return timestamp;
}

const RawData &File::get_data() const {
    return data;
}

const size_t File::get_size() const {
    return data.get_size();
}

};
};
