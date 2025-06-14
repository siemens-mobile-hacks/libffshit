#include "ffshit/filesystem/structure/directory.h"

namespace FULLFLASH {
namespace Filesystem {

Directory::Directory(std::string name) : name(name) { }

Directory::Ptr Directory::build(std::string name) {
    return std::make_shared<Directory>(name);
}

void Directory::add_subdir(Ptr dir) {
    subdirs.push_back(dir);
}

void Directory::add_file(File::Ptr file) {
    files.push_back(file);
}

const std::string Directory::get_name() const {
    return name;
}

const Directory::Directories & Directory::get_subdirs() const {
    return subdirs;
}

const File::Files & Directory::get_files() const {
    return files;
}

};
};
