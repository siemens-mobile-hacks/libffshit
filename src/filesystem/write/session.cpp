#include "filesystem/write/session.h"

#include "ffshit/filesystem/ex.h"
#include "ffshit/filesystem/help.h"

#include <sstream>

namespace FULLFLASH {
namespace Filesystem {
namespace Write {

Session::Session(Platform::Type platform, Partitions::Partitions::Ptr partitions, Directory::Ptr root) :
    platform(platform),
    partitions(partitions),
    root(root) {
}

Session::Path Session::parse(const std::string &path) const {
    std::stringstream           stream(path);
    std::string                 name;
    std::vector<std::string>    names;

    while (std::getline(stream, name, '/')) {
        if (!name.empty()) {
            names.push_back(name);
        }
    }

    if (names.empty() || names.front().find("FFS") == std::string::npos || !partitions->get_partitions().count(names.front())) {
        throw Exception("'{}': no such partition", path);
    }

    if (names.size() == 1) {
        throw Exception("'{}' is a partition's root directory", path);
    }

    return { names.front(), std::vector<std::string>(names.begin() + 1, names.end()) };
}

Writer &Session::writer(const std::string &partition) {
    auto it = writers.find(partition);

    if (it == writers.end()) {
        it = writers.emplace(partition, Writer::build(platform, partitions, partition)).first;
    }

    return *it->second;
}

void Session::run(const Path &path, const std::function<void(Writer &)> &operation) {
    Writer &    writer  = this->writer(path.partition);
    Records &   records = writer.get_records();

    records.begin();

    try {
        operation(writer);
    } catch (...) {
        records.rollback();

        throw;
    }

    records.commit();

    partitions->reload_block_data(path.partition);
}

Directory::Ptr Session::loaded_parent(const Writer &writer, const Path &path, std::string &entries_path) const {
    Directory::Ptr directory;

    for (const auto &partition : root->get_subdirs()) {
        if (partition->get_name() == path.partition) {
            directory = partition;
        }
    }

    entries_path = path.partition + "/";

    for (size_t i = 0; directory && i + 1 < path.names.size(); ++i) {
        Directory::Ptr next;

        for (const auto &subdir : directory->get_subdirs()) {
            if (writer.same_name(subdir->get_name(), path.names[i])) {
                next = subdir;
            }
        }

        directory = next;

        if (directory) {
            entries_path += directory->get_name() + "/";
        }
    }

    return directory;
}

void Session::write_file(const std::string &path, const RawData &data, const TimePoint &timestamp) {
    Path        parsed      = parse(path);
    uint32_t    fat_time    = unix_to_fat_timestamp(timestamp);

    run(parsed, [&](Writer &writer) {
        writer.write_file(parsed.names, data, fat_time);
    });

    const Writer &      writer  = this->writer(parsed.partition);
    std::string         entries_path;
    Directory::Ptr      parent  = loaded_parent(writer, parsed, entries_path);

    if (!parent) {
        return;
    }

    for (const auto &file : File::Files(parent->get_files())) {
        if (writer.same_name(file->get_name(), parsed.names.back())) {
            parent->remove_file(file);
        }
    }

    parent->add_file(File::build(parsed.names.back(), entries_path, data, Attributes(0), fat_timestamp_to_unix(fat_time)));
}

void Session::create_directory(const std::string &path, const TimePoint &timestamp) {
    Path        parsed      = parse(path);
    uint32_t    fat_time    = unix_to_fat_timestamp(timestamp);

    run(parsed, [&](Writer &writer) {
        writer.create_directory(parsed.names, fat_time);
    });

    std::string     entries_path;
    Directory::Ptr  parent = loaded_parent(writer(parsed.partition), parsed, entries_path);

    if (parent) {
        Attributes attributes(static_cast<uint32_t>(FileAttributes::DIRECTORY));

        parent->add_subdir(Directory::build(parsed.names.back(), entries_path, attributes, fat_timestamp_to_unix(fat_time)));
    }
}

void Session::remove(const std::string &path) {
    Path parsed = parse(path);

    run(parsed, [&](Writer &writer) {
        writer.remove(parsed.names);
    });

    const Writer &      writer  = this->writer(parsed.partition);
    std::string         entries_path;
    Directory::Ptr      parent  = loaded_parent(writer, parsed, entries_path);

    if (!parent) {
        return;
    }

    for (const auto &file : File::Files(parent->get_files())) {
        if (writer.same_name(file->get_name(), parsed.names.back())) {
            parent->remove_file(file);
        }
    }

    for (const auto &subdir : Directory::Directories(parent->get_subdirs())) {
        if (writer.same_name(subdir->get_name(), parsed.names.back())) {
            parent->remove_subdir(subdir);
        }
    }
}

};
};
};
