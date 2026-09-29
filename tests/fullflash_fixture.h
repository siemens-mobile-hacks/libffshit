#ifndef LIBFFSHIT_TESTS_FULLFLASH_FIXTURE_H
#define LIBFFSHIT_TESTS_FULLFLASH_FIXTURE_H

#include <ffshit/fullflash.h>
#include <ffshit/filesystem/platform/builder.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>

namespace Tests {

// A fullflash of the collection the tests run on
struct Phone {
    std::string                         name;
    std::string                         fullflash;
    FULLFLASH::Platform::Type           platform;
    // the partition the phone shows as /Data, and a directory in it the tests write into
    std::string                         partition;
    std::string                         dir;
};

std::ostream &                          operator<<(std::ostream &os, const Phone &phone);

// Where the fullflash is, or an empty path when it is not there
std::filesystem::path                   find_fullflash(const std::string &file_name);

struct Loaded {
    FULLFLASH::FULLFLASH::Ptr           fullflash;
    FULLFLASH::Filesystem::Base::Ptr    filesystem;
};

// SGOLD file names are read in the codepage
Loaded                                  load(const std::filesystem::path &path, const std::string &codepage = "CP1252");

// What a file or directory looked like: its content (empty for a directory) and timestamp
struct Entry {
    bool                                is_directory;
    std::string                         data;
    int64_t                             timestamp;

    bool operator==(const Entry &other) const {
        return is_directory == other.is_directory && data == other.data && timestamp == other.timestamp;
    }
};

// Every file and directory under the root, by path, e.g. "FFS_0/Misc/photo.jpg"
using Tree = std::map<std::string, Entry>;

Tree                                    snapshot(const FULLFLASH::Filesystem::Base &filesystem);

FULLFLASH::Filesystem::File::Ptr        find_file(const FULLFLASH::Filesystem::Base &filesystem, const std::string &path);
FULLFLASH::Filesystem::Directory::Ptr   find_directory(const FULLFLASH::Filesystem::Base &filesystem, const std::string &path);

std::string                             to_string(const FULLFLASH::RawData &data);
FULLFLASH::RawData                      to_raw_data(const std::string &data);

// Content that tells a shifted or truncated copy from the original
std::string                             pattern(size_t size, uint8_t seed);

// A timestamp a FAT timestamp can hold: whole even seconds
FULLFLASH::Filesystem::TimePoint        timestamp(int year, int month, int day, int hour, int minute, int second);

class FullflashTest : public ::testing::TestWithParam<Phone> {
    protected:
        std::filesystem::path           original_path;
        std::filesystem::path           temp_dir;
        Loaded                          original;

        void                            SetUp() override;
        void                            TearDown() override;

        // Saves the fullflash and reads it back from the file
        Loaded                          save_and_reload(const Loaded &loaded, const std::string &codepage = "CP1252");

        std::string                     dir_path(const std::string &name = "") const;
};

};

#endif
