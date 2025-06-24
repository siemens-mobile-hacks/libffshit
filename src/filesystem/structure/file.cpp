#include "ffshit/filesystem/structure/file.h"

namespace FULLFLASH {
namespace Filesystem {

File::File(const std::string name, const std::string path, const RawData &data) : 
    name(name),
    path(path),
    timestamp(std::chrono::seconds(0)),
    data(data) {
}

File::File(const std::string name, const std::string path, const TimePoint &timestamp, const RawData &data) : 
    name(name),
    path(path),
    timestamp(timestamp),
    data(data) {
}

File::Ptr File::build(const std::string name, const std::string path, const RawData &data) {
    return std::make_shared<File>(name, path, data);
}

File::Ptr File::build(const std::string name, const std::string path, const TimePoint &timestamp, const RawData &data) {
    return std::make_shared<File>(name, path, timestamp, data);
}

const std::string &File::get_name() const {
    return name;
}

const std::string &File::get_path() const {
    return path;
}

const TimePoint File::get_timestamp() const {
    return timestamp;
}

RawData File::get_data() const {
    return data;
}

const size_t File::get_size() const {
    return data.get_size();
}

};
};
