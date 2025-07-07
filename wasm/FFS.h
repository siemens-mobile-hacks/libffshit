#pragma once

#include <ffshit/ex.h>
#include <ffshit/filesystem/structure/directory.h>
#include <ffshit/filesystem/structure/structure.h>
#include <ffshit/version.h>
#include <ffshit/system.h>
#include <ffshit/log/logger.h>
#include <ffshit/filesystem/ex.h>
#include <ffshit/partition/partitions.h>
#include <ffshit/partition/ex.h>
#include <ffshit/filesystem/ex.h>
#include <ffshit/filesystem/platform/types.h>
#include <ffshit/filesystem/platform/builder.h>

class FFSLogInterface : public FULLFLASH::Log::Interface {
public:
    using Ptr = std::shared_ptr<FFSLogInterface>;

    FFSLogInterface() { }

    static Ptr build() {
        return std::make_shared<FFSLogInterface>();
    }

    void on_info(std::string msg) override final {
        fprintf(stderr, "[FFS] [I] %s\n", msg.c_str());
    }

    void on_warning(std::string msg) override final {
        fprintf(stderr, "[FFS] [W] %s\n", msg.c_str());
    }

    void on_error(std::string msg) override final {
        fprintf(stderr, "[FFS] [E] %s\n", msg.c_str());
    }

    void on_debug(std::string msg) override final {
        fprintf(stderr, "[FFS] [D] %s\n", msg.c_str());
    }
};

class FFS {
public:
    struct Options {
        bool isOldSearchAlgorithm = false;
        bool skipBroken = true;
        bool skipDuplicates = true;
        uint32_t searchStartAddress = 0;
        std::string platform;
        bool debug = false;
    };

    struct Entry {
        std::string name;
        std::string path;
        double timestamp = 0; // lol
        size_t size = 0;
        bool isFile = false;
        bool isDirectory = false;
    };

    struct DirOrFile {
        FULLFLASH::Filesystem::Directory::Ptr dir = nullptr;
        FULLFLASH::Filesystem::File::Ptr file = nullptr;
    };

    struct FileData {
        uintptr_t data;
        size_t size;
    };

private:
    FULLFLASH::Filesystem::Directory::Ptr m_rootDir;
    FULLFLASH::Partitions::Partitions::Ptr m_partitions;
    FULLFLASH::Filesystem::Base::Ptr m_filesystem;
    FULLFLASH::Platform m_platform = FULLFLASH::Platform::UNK;
    FULLFLASH::Filesystem::FSMap m_map;

    FULLFLASH::Filesystem::Directory::Ptr getDirPtr(const std::string &path);
    DirOrFile getDirOrFilePtr(const std::string &path);

public:
    FFS();
    ~FFS();

    void open(uintptr_t ptr, size_t size, const Options &options);
    std::string getPlatform();
    std::string getModel();
    std::string getIMEI();
    Entry stat(const std::string &path);
    std::vector<Entry> readDir(const std::string &path);
    FileData readFile(const std::string &path);

    void close();
};
