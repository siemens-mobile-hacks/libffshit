#include "ffshit/filesystem/platform/base.h"
#include "ffshit/filesystem/ex.h"

#include "filesystem/write/session.h"

namespace FULLFLASH {
namespace Filesystem {

void Base::write_file(const std::string &path, const RawData &data, const TimePoint &timestamp) {
    throw Exception("Writing is not supported on this platform");
}

void Base::create_directory(const std::string &path, const TimePoint &timestamp) {
    throw Exception("Writing is not supported on this platform");
}

void Base::remove(const std::string &path) {
    throw Exception("Writing is not supported on this platform");
}

Write::Session &Base::write_session(Platform::Type platform, Partitions::Partitions::Ptr partitions, Directory::Ptr root) {
    if (!session) {
        session = std::make_shared<Write::Session>(platform, partitions, root);
    }

    return *session;
}

};
};
