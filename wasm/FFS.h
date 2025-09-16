#pragma once

#include <ffshit/ex.h>
#include <ffshit/filesystem/structure/directory.h>
#include <ffshit/filesystem/structure/structure.h>
#include <ffshit/log/logger.h>
#include <ffshit/partition/partitions.h>
#include <ffshit/platform/types.h>
#include <ffshit/filesystem/platform/builder.h>

class FFSLogInterface final : public FULLFLASH::Log::Interface {
public:
    virtual ~FFSLogInterface() = default;
    using Ptr = std::shared_ptr<FFSLogInterface>;

    FFSLogInterface() = default;

    static Ptr build() {
        return std::make_shared<FFSLogInterface>();
    }

    void on_info(const std::string msg) final {
        fprintf(stderr, "[FFS] [I] %s\n", msg.c_str());
    }

    void on_warning(const std::string msg) final {
        fprintf(stderr, "[FFS] [W] %s\n", msg.c_str());
    }

    void on_error(const std::string msg) final {
        fprintf(stderr, "[FFS] [E] %s\n", msg.c_str());
    }

    void on_debug(const std::string msg) final {
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
        bool verboseProcessing = false;
        bool verboseHeaders = false;
        bool verboseData = false;
    };

    struct Entry {
        std::string name;
        std::string path;
        double timestamp = 0; // lol
        size_t size = 0;
        bool isFile = false;
        bool isDirectory = false;
        bool isReadonly = false;
        bool isHidden = false;
        bool isSystem = false;
    };

    struct DirOrFile {
        FULLFLASH::Filesystem::Directory::Ptr dir = nullptr;
        FULLFLASH::Filesystem::File::Ptr file = nullptr;
        std::string canonicalPath;
    };

    struct FileData {
        uintptr_t data;
        size_t size;
    };

private:
    FULLFLASH::FULLFLASH::Ptr m_fullflash;
    FULLFLASH::Filesystem::Directory::Ptr m_rootDir;
    FULLFLASH::Partitions::Partitions::Ptr m_partitions;
    FULLFLASH::Filesystem::Base::Ptr m_filesystem;
    FULLFLASH::Platform::Type m_platform = FULLFLASH::Platform::Type::UNK;

    std::tuple<FULLFLASH::Filesystem::Directory::Ptr, std::string> getDirPtr(const std::string &path) const;
    DirOrFile getDirOrFilePtr(const std::string &path) const;
    static void fillAttributes(Entry *entry, const FULLFLASH::Filesystem::File::Ptr& file, const std::string &parentPath) ;
    void fillAttributes(Entry *entry, const FULLFLASH::Filesystem::Directory::Ptr& dir, const std::string &parentPath) const;
public:
    FFS();
    ~FFS();

    void open(uintptr_t ptr, size_t size, const Options &options);
    std::string getPlatform() const;
    std::string getModel() const;
    std::string getIMEI() const;
    Entry stat(const std::string &path) const;
    std::vector<Entry> readDir(const std::string &path) const;
    FileData readFile(const std::string &path) const;
    void close();
};
