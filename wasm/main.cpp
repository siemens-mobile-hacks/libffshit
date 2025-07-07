#include <cstdint>
#include <ffshit/filesystem/structure/directory.h>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <emscripten/bind.h>
#include <ffshit/filesystem/platform/types.h>

#include "FFS.h"

using namespace emscripten;

static int64_t timePointToUnix(std::chrono::system_clock::time_point tp);
static std::string joinPath(const std::vector<std::string> &parts);
static std::vector<std::string> splitPath(const std::string &input);
static std::string getParentDir(const std::string &path);
static std::string getBaseName(const std::string &path);
static std::string toLower(const std::string &s);

EMSCRIPTEN_BINDINGS(libffshit) {
    class_<FFS>("FFS")
        .constructor<>()
        .function("getPlatform", &FFS::getPlatform)
        .function("getIMEI", &FFS::getIMEI)
        .function("getModel", &FFS::getModel)
        .function("readDir", &FFS::readDir)
        .function("readFile", &FFS::readFile)
        .function("open", &FFS::open)
        .function("stat", &FFS::stat)
        .function("close", &FFS::close);
    register_vector<FFS::Entry>("EntryArray");

    value_object<FFS::FileData>("FileData")
        .field("data", &FFS::FileData::data)
        .field("size", &FFS::FileData::size);

    value_object<FFS::Entry>("Entry")
        .field("name", &FFS::Entry::name)
        .field("path", &FFS::Entry::path)
        .field("size", &FFS::Entry::size)
        .field("timestamp", &FFS::Entry::timestamp)
        .field("isFile", &FFS::Entry::isFile)
        .field("isDirectory", &FFS::Entry::isDirectory);

    value_object<FFS::Options>("Options")
        .field("isOldSearchAlgorithm", &FFS::Options::isOldSearchAlgorithm)
        .field("searchStartAddress", &FFS::Options::searchStartAddress)
        .field("platform", &FFS::Options::platform)
        .field("skipBroken", &FFS::Options::skipBroken)
        .field("skipDuplicates", &FFS::Options::skipDuplicates);
};

FFS::FFS() {

}

void FFS::open(uintptr_t ptr, size_t size, const FFS::Options &options) {
    try {
        char *data = reinterpret_cast<char *>(ptr);
        m_partitions = FULLFLASH::Partitions::Partitions::build(data, size, options.isOldSearchAlgorithm, options.searchStartAddress);
        m_platform = m_partitions->get_platform();

        if (options.platform != "auto")
            m_platform = FULLFLASH::StringToPlatform.at(options.platform);
        m_filesystem = FULLFLASH::Filesystem::build(m_platform, m_partitions);

        m_filesystem->load(options.skipBroken, options.skipDuplicates);

        m_rootDir = FULLFLASH::Filesystem::Directory::build("", "");
        for (const auto &it: m_filesystem->get_filesystem_map()) {
            auto disk = FULLFLASH::Filesystem::Directory::build(it.first, "/", it.second->get_timestamp());
            for (const auto &subdir: it.second->get_subdirs())
                disk->add_subdir(subdir);
            m_rootDir->add_subdir(disk);
        }
    } catch (const FULLFLASH::Partitions::Exception &e) {
        close();
        throw std::runtime_error("[FULLFLASH::Partitions::Exception] " + e.what());
    } catch (const FULLFLASH::Filesystem::Exception &e) {
        close();
        throw std::runtime_error("[FULLFLASH::Filesystem::Exception] " + e.what());
    } catch (const FULLFLASH::Exception &e) {
        close();
        throw std::runtime_error("[FULLFLASH::Exception] " + e.what());
    } catch (const FULLFLASH::BaseException &e) {
        close();
        throw std::runtime_error("[FULLFLASH::BaseException] " + e.what());
    } catch(...) {
        close();
        throw;
    }
}

void FFS::close() {
    m_filesystem.reset();
    m_partitions.reset();
}

FFS::~FFS() {
    close();
}

std::string FFS::getPlatform() {
    if (!m_partitions)
        throw std::runtime_error("FFS is closed.");
    return FULLFLASH::PlatformToString.at(m_partitions->get_platform());
}

std::string FFS::getIMEI() {
    if (!m_partitions)
        throw std::runtime_error("FFS is closed.");
    return m_partitions->get_imei();
}

std::string FFS::getModel() {
    if (!m_partitions)
        throw std::runtime_error("FFS is closed.");
    return m_partitions->get_model();
}

FFS::FileData FFS::readFile(const std::string &path) {
    const auto dirOrFile = getDirOrFilePtr(path);
    if (!dirOrFile.file)
        return { .data = 0, .size = 0 };
    const auto data = dirOrFile.file->get_data().get_data().get();
    auto dataSize = dirOrFile.file->get_data().get_size();
    return { .data = reinterpret_cast<uintptr_t>(data), .size = dataSize };
}

FULLFLASH::Filesystem::Directory::Ptr FFS::getDirPtr(const std::string &path) {
    FULLFLASH::Filesystem::Directory::Ptr dir = m_rootDir;
    const auto parts = splitPath(path);
    for (const auto &part: parts) {
        bool found = false;
        for (const auto subdir: dir->get_subdirs()) {
            if (toLower(subdir->get_name()) == part) {
                dir = subdir;
                found = true;
                break;
            }
        }
        if (!found)
            return nullptr;
    }
    return dir;
}

FFS::Entry FFS::stat(const std::string &path) {
    const auto dirOrFile = getDirOrFilePtr(getParentDir(path));
    if (dirOrFile.dir) {
        Entry entry = {};
        entry.name = dirOrFile.dir->get_name();
        entry.path = dirOrFile.dir->get_path();
        entry.timestamp = timePointToUnix(dirOrFile.dir->get_timestamp()) * 1000;
        entry.isDirectory = true;
        return entry;
    } else if (dirOrFile.file) {
        Entry entry = {};
        entry.name = dirOrFile.file->get_name();
        entry.path = dirOrFile.file->get_path();
        entry.timestamp = timePointToUnix(dirOrFile.file->get_timestamp()) * 1000;
        entry.size = dirOrFile.file->get_size();
        entry.isFile = true;
        return entry;
    }
    return {};
}

FFS::DirOrFile FFS::getDirOrFilePtr(const std::string &path) {
    const auto parentDir = getDirPtr(getParentDir(path));
    const auto baseName = getBaseName(path);
    for (const auto subdir: parentDir->get_subdirs()) {
       if (toLower(subdir->get_name()) == baseName)
           return { .dir = subdir };
    }
    for (const auto file: parentDir->get_files()) {
        if (toLower(file->get_name()) == baseName)
            return { .file = file };
    }
    return {};
}

std::vector<FFS::Entry> FFS::readDir(const std::string &path) {
    const auto parentDir = getDirPtr(path);
    std::vector<Entry> entries;
    if (parentDir) {
        for (const auto subdir: parentDir->get_subdirs()) {
            entries.resize(entries.size() + 1);
            auto &entry = entries.back();
            entry.name = subdir->get_name();
            entry.path = subdir->get_path();
            entry.timestamp = timePointToUnix(subdir->get_timestamp()) * 1000;
            entry.isDirectory = true;
        }
        for (const auto file: parentDir->get_files()) {
            entries.resize(entries.size() + 1);
            auto &entry = entries.back();
            entry.name = file->get_name();
            entry.path = file->get_path();
            entry.timestamp = timePointToUnix(file->get_timestamp()) * 1000;
            entry.size = file->get_size();
            entry.isFile = true;
        }
    }
    return entries;
}

static std::string getBaseName(const std::string &path) {
    const auto parts = splitPath(path);
    return parts.empty() ? "" : parts.back();
}

static std::string getParentDir(const std::string &path) {
    auto parts = splitPath(path);
    if (!parts.empty())
        parts.pop_back();
    return joinPath(parts);
}

static std::vector<std::string> splitPath(const std::string &input) {
    if (input.empty() || input[0] != '/')
        throw std::invalid_argument("Path must be absolute");

    std::vector<std::string> parts;
    std::string part;
    size_t i = 1;

    for (; i <= input.size(); ++i) {
        if (i == input.size() || input[i] == '/') {
            if (!part.empty()) {
                if (part == ".") {
                    // skip
                } else if (part == "..") {
                    if (!parts.empty()) {
                        parts.pop_back();
                    }
                } else {
                    parts.push_back(toLower(part));
                }
                part.clear();
            }
            while (i + 1 < input.size() && input[i + 1] == '/')
                i++;
        } else {
            part += input[i];
        }
    }

    return parts;
}

static std::string joinPath(const std::vector<std::string> &parts) {
    if (parts.empty())
        return "/";
    std::string result;
    for (const auto &part: parts)
        result += "/" + part;
    return result;
}

static int64_t timePointToUnix(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

static std::string toLower(const std::string &s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), [](char c) {
        return std::tolower(static_cast<unsigned char>(c));
    });
    return result;
}
