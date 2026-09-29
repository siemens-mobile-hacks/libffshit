#include "fullflash_fixture.h"

#include <cstdlib>
#include <ctime>
#include <random>
#include <sstream>

namespace Tests {

std::ostream &operator<<(std::ostream &os, const Phone &phone) {
    return os << phone.name;
}

std::filesystem::path find_fullflash(const std::string &file_name) {
    std::vector<std::filesystem::path> dirs;

    if (const char *env = std::getenv("FFSHIT_TEST_FULLFLASHES")) {
        dirs.push_back(env);
    }

    dirs.push_back(FFSHIT_TEST_FULLFLASHES_DIR);

    for (const auto &dir : dirs) {
        std::filesystem::path path = dir / file_name;

        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return {};
}

Loaded load(const std::filesystem::path &path) {
    Loaded loaded;

    loaded.fullflash = FULLFLASH::FULLFLASH::build(path);
    loaded.fullflash->load_partitions(false, 0);

    auto partitions = loaded.fullflash->get_partitions();

    loaded.filesystem = FULLFLASH::Filesystem::build(partitions->get_fs_platform(), partitions);
    loaded.filesystem->load();

    return loaded;
}

static void add_entry(Tree &tree, std::string path, const Entry &entry) {
    std::string key = path;

    for (size_t copy = 2; tree.count(key); ++copy) {
        key = path + "#" + std::to_string(copy);
    }

    tree[key] = entry;
}

static int64_t seconds(const FULLFLASH::Filesystem::TimePoint &time_point) {
    return std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count();
}

static void snapshot_dir(Tree &tree, const FULLFLASH::Filesystem::Directory::Ptr &dir, const std::string &path) {
    for (const auto &file : dir->get_files()) {
        add_entry(tree, path + "/" + file->get_name(), { false, to_string(file->get_data()), seconds(file->get_timestamp()) });
    }

    for (const auto &subdir : dir->get_subdirs()) {
        std::string subdir_path = path + "/" + subdir->get_name();

        add_entry(tree, subdir_path, { true, "", seconds(subdir->get_timestamp()) });
        snapshot_dir(tree, subdir, subdir_path);
    }
}

Tree snapshot(const FULLFLASH::Filesystem::Base &filesystem) {
    Tree tree;

    for (const auto &partition : filesystem.get_root()->get_subdirs()) {
        add_entry(tree, partition->get_name(), { true, "", seconds(partition->get_timestamp()) });
        snapshot_dir(tree, partition, partition->get_name());
    }

    return tree;
}

static std::vector<std::string> split_path(const std::string &path) {
    std::vector<std::string>    parts;
    std::stringstream           stream(path);
    std::string                 part;

    while (std::getline(stream, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    return parts;
}

static FULLFLASH::Filesystem::Directory::Ptr find_subdir(const FULLFLASH::Filesystem::Directory::Ptr &dir, const std::string &name) {
    for (const auto &subdir : dir->get_subdirs()) {
        if (subdir->get_name() == name) {
            return subdir;
        }
    }

    return nullptr;
}

FULLFLASH::Filesystem::Directory::Ptr find_directory(const FULLFLASH::Filesystem::Base &filesystem, const std::string &path) {
    auto dir = filesystem.get_root();

    for (const auto &part : split_path(path)) {
        dir = find_subdir(dir, part);

        if (!dir) {
            return nullptr;
        }
    }

    return dir;
}

FULLFLASH::Filesystem::File::Ptr find_file(const FULLFLASH::Filesystem::Base &filesystem, const std::string &path) {
    auto parts = split_path(path);

    if (parts.empty()) {
        return nullptr;
    }

    std::string name = parts.back();

    parts.pop_back();

    auto dir = filesystem.get_root();

    for (const auto &part : parts) {
        dir = find_subdir(dir, part);

        if (!dir) {
            return nullptr;
        }
    }

    for (const auto &file : dir->get_files()) {
        if (file->get_name() == name) {
            return file;
        }
    }

    return nullptr;
}

std::string to_string(const FULLFLASH::RawData &data) {
    if (data.get_size() == 0) {
        return "";
    }

    return std::string(data.get_data().get(), data.get_size());
}

FULLFLASH::RawData to_raw_data(const std::string &data) {
    if (data.empty()) {
        return FULLFLASH::RawData();
    }

    return FULLFLASH::RawData(const_cast<char *>(data.data()), data.size());
}

std::string pattern(size_t size, uint8_t seed) {
    std::string data(size, '\0');

    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>((i * 31 + (i >> 8) + seed * 7) & 0xFF);
    }

    return data;
}

FULLFLASH::Filesystem::TimePoint timestamp(int year, int month, int day, int hour, int minute, int second) {
    std::tm tm{};

    tm.tm_year  = year - 1900;
    tm.tm_mon   = month - 1;
    tm.tm_mday  = day;
    tm.tm_hour  = hour;
    tm.tm_min   = minute;
    tm.tm_sec   = second;
    tm.tm_isdst = -1;

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

void FullflashTest::SetUp() {
    const Phone &phone = GetParam();

    original_path = find_fullflash(phone.fullflash);

    if (original_path.empty()) {
        GTEST_SKIP() << phone.fullflash << " not found. Point FFSHIT_TEST_FULLFLASHES at a directory holding it";
    }

    static const auto   run     = std::random_device()();
    static size_t       counter = 0;

    temp_dir = std::filesystem::temp_directory_path() / ("ffshit-tests-" + std::to_string(run) + "-" + std::to_string(counter++));
    std::filesystem::create_directories(temp_dir);

    original = load(original_path);

    ASSERT_EQ(original.fullflash->get_partitions()->get_fs_platform(), phone.platform);
    ASSERT_NE(find_directory(*original.filesystem, dir_path()), nullptr) << dir_path();
}

void FullflashTest::TearDown() {
    if (!temp_dir.empty()) {
        std::filesystem::remove_all(temp_dir);
    }
}

Loaded FullflashTest::save_and_reload(const Loaded &loaded, const std::string &file_name) {
    std::filesystem::path path = temp_dir / file_name;

    loaded.fullflash->save(path);

    return load(path);
}

std::string FullflashTest::dir_path(const std::string &name) const {
    std::string path = GetParam().partition + "/" + GetParam().dir;

    if (!name.empty()) {
        path += "/" + name;
    }

    return path;
}

};
