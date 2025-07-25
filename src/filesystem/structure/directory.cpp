#include "ffshit/filesystem/structure/directory.h"

namespace FULLFLASH {
namespace Filesystem {

Directory::Directory(const std::string name, const std::string path) :
    name(name),
    path(path),
    attributes(Attributes(static_cast<uint32_t>(FileAttributes::DIRECTORY))),
    timestamp(std::chrono::seconds(0)) {
}

Directory::Directory(const std::string name, const std::string path, const Attributes &attributes) :
    name(name),
    path(path),
    attributes(attributes),
    timestamp(std::chrono::seconds(0)) {
}

Directory::Directory(const std::string name, const std::string path, const Attributes &attributes, const TimePoint &timestamp) :
    name(name),
    path(path),
    attributes(attributes),
    timestamp(timestamp) {
}

Directory::Ptr Directory::build(const std::string name, const std::string path) {
    return std::make_shared<Directory>(name, path);
}

Directory::Ptr Directory::build(const std::string name, const std::string path, const Attributes &attributes) {
    return std::make_shared<Directory>(name, path, attributes);
}

Directory::Ptr Directory::build(const std::string name, const std::string path, const Attributes &attributes, const TimePoint &timestamp) {
    return std::make_shared<Directory>(name, path, attributes, timestamp);
}

void Directory::add_subdir(Ptr dir) {
    subdirs.push_back(dir);
}

void Directory::add_file(File::Ptr file) {
    files.push_back(file);
}

const std::string &Directory::get_name() const {
    return name;
}

const std::string &Directory::get_path() const {
    return path;
}

const Attributes &Directory::get_attributes() const {
    return attributes;
}

const TimePoint Directory::get_timestamp() const {
    return timestamp;
}

const Directory::Directories & Directory::get_subdirs() const {
    return subdirs;
}

const File::Files & Directory::get_files() const {
    return files;
}

};
};
