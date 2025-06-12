#include "ffshit/filesystem/file.h"

namespace FULLFLASH {
namespace Filesystem {

File::File(const std::string name, const RawData &data) : name(name), data(data) {

}

File::Ptr File::build(const std::string name, const RawData &data) {
    return std::make_shared<File>(name, data);
}

const std::string File::get_name() const {
    return name;
}

RawData File::get_data() const {
    return data;
}

const size_t File::get_size() const {
    return data.get_size();
}
};
};
