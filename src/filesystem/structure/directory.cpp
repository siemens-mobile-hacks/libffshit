#include "ffshit/filesystem/structure/directory.h"

namespace FULLFLASH {
namespace Filesystem {

Directory::Directory(const std::string name, const std::string path) : 
    name(name),
    path(path) { 

}

Directory::Ptr Directory::build(const std::string name, const std::string path) {
    return std::make_shared<Directory>(name, path);
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

const Directory::Directories & Directory::get_subdirs() const {
    return subdirs;
}

const File::Files & Directory::get_files() const {
    return files;
}

};
};
