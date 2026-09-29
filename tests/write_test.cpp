// Writes to the filesystem of a fullflash, saves it, reads it back and checks that the files are
// there and that nothing else changed.

#include "fullflash_fixture.h"

#include <ffshit/ex.h>

#include <fstream>

using namespace Tests;
using FULLFLASH::Platform::Type;

namespace {

const std::vector<Phone> PHONES = {
    { "CX70", "CX70v56lg3.bin",     Type::SGOLD,       "FFS",   "Misc" },
    { "SL65", "SL65v49lg1_TIM.bin", Type::SGOLD,       "FFS",   "Misc" },
    { "S75",  "S75v40lg1.bin",      Type::SGOLD2,      "FFS_0", "Misc" },
    { "EL71", "EL71v41lg91.bin",    Type::SGOLD2_ELKA, "FFS_0", "Misc" },
};

const FULLFLASH::Filesystem::TimePoint WRITE_TIME = timestamp(2024, 5, 17, 13, 37, 42);

int64_t seconds(const FULLFLASH::Filesystem::TimePoint &time_point) {
    return std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count();
}

std::string read_file(const std::filesystem::path &path) {
    std::ifstream   file(path, std::ios_base::binary);
    std::string     data(std::filesystem::file_size(path), '\0');

    file.read(&data[0], data.size());

    return data;
}

Entry file_entry(const std::string &data) {
    return { false, data, seconds(WRITE_TIME) };
}

Entry dir_entry() {
    return { true, "", seconds(WRITE_TIME) };
}

// Lists the differences instead of dumping two trees of a thousand files
void expect_tree(const Tree &actual, const Tree &expected) {
    std::vector<std::string> differences;

    for (const auto &pair : expected) {
        auto it = actual.find(pair.first);

        if (it == actual.end()) {
            differences.push_back("missing:   " + pair.first);
        } else if (!(it->second == pair.second)) {
            differences.push_back(fmt::format("different: {} ({} bytes at {}, expected {} bytes at {})",
                pair.first, it->second.data.size(), it->second.timestamp, pair.second.data.size(), pair.second.timestamp));
        }
    }

    for (const auto &pair : actual) {
        if (!expected.count(pair.first)) {
            differences.push_back("unexpected: " + pair.first);
        }
    }

    std::string report;

    for (size_t i = 0; i < differences.size() && i < 20; ++i) {
        report += differences[i] + "\n";
    }

    EXPECT_TRUE(differences.empty()) << differences.size() << " differences:\n" << report;
}

// The first file with content somewhere in the partition
std::string firmware_file(const Tree &tree, const std::string &partition) {
    for (const auto &pair : tree) {
        const std::string &path = pair.first;

        if (path.rfind(partition + "/", 0) == 0 && !pair.second.is_directory && !pair.second.data.empty() &&
            path.find('#') == std::string::npos) {
            return path;
        }
    }

    return "";
}

// The first directory in the partition with something in it
std::string firmware_directory(const Tree &tree, const std::string &partition) {
    for (const auto &pair : tree) {
        const std::string &path = pair.first;

        if (path.rfind(partition + "/", 0) == 0 && pair.second.is_directory) {
            auto next = tree.upper_bound(path + "/");

            if (next != tree.end() && next->first.rfind(path + "/", 0) == 0) {
                return path;
            }
        }
    }

    return "";
}

}

class WriteTest : public FullflashTest {
    protected:
        void write(const std::string &path, const std::string &data) {
            original.filesystem->write_file(path, to_raw_data(data), WRITE_TIME);
        }

        void expect_unchanged_on_disk() {
            std::filesystem::path path = temp_dir / "unchanged.bin";

            original.fullflash->save(path);

            EXPECT_TRUE(read_file(path) == read_file(original_path)) << "the fullflash changed";
        }
};

TEST_P(WriteTest, SavesAnUnchangedFullflashByteForByte) {
    expect_unchanged_on_disk();
}

TEST_P(WriteTest, WritesFilesOfEverySize) {
    // Around the chunk sizes (1024, 2048, 4096) and ELKA's split between inline records (up to
    // 512 bytes) and the data area (in 1 KiB units)
    const std::vector<size_t> sizes = {
        0, 1, 15, 16, 17, 100, 511, 512, 513, 1023, 1024, 1025, 1176, 1535, 1536, 1537,
        2047, 2048, 2049, 3000, 4095, 4096, 4097, 10000, 70000,
    };

    Tree expected = snapshot(*original.filesystem);

    for (size_t i = 0; i < sizes.size(); ++i) {
        std::string path = dir_path(fmt::format("ffshit-{}.bin", sizes[i]));
        std::string data = pattern(sizes[i], i);

        write(path, data);
        expected[path] = file_entry(data);
    }

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, CreatesDirectories) {
    Tree expected = snapshot(*original.filesystem);

    original.filesystem->create_directory(dir_path("ffshit-dir"), WRITE_TIME);
    original.filesystem->create_directory(dir_path("ffshit-dir/sub"), WRITE_TIME);
    write(dir_path("ffshit-dir/a.bin"), pattern(3000, 1));
    write(dir_path("ffshit-dir/sub/b.bin"), pattern(5, 2));

    expected[dir_path("ffshit-dir")]            = dir_entry();
    expected[dir_path("ffshit-dir/sub")]        = dir_entry();
    expected[dir_path("ffshit-dir/a.bin")]      = file_entry(pattern(3000, 1));
    expected[dir_path("ffshit-dir/sub/b.bin")]  = file_entry(pattern(5, 2));

    Loaded reloaded = save_and_reload(original);

    expect_tree(snapshot(*reloaded.filesystem), expected);

    auto dir = find_directory(*reloaded.filesystem, dir_path("ffshit-dir"));

    ASSERT_NE(dir, nullptr);
    EXPECT_TRUE(dir->get_attributes().is_directory());
}

TEST_P(WriteTest, ReplacesAFile) {
    Tree expected = snapshot(*original.filesystem);

    write(dir_path("ffshit-replaced.bin"), pattern(5000, 1));
    write(dir_path("ffshit-replaced.bin"), pattern(300, 2));

    expected[dir_path("ffshit-replaced.bin")] = file_entry(pattern(300, 2));

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, ReplacesAFileOfTheFirmware) {
    Tree        expected    = snapshot(*original.filesystem);
    std::string path        = firmware_file(expected, GetParam().partition);

    ASSERT_FALSE(path.empty());

    write(path, pattern(777, 3));

    expected[path] = file_entry(pattern(777, 3));

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, MatchesNamesWithoutCase) {
    Tree expected = snapshot(*original.filesystem);

    write(dir_path("ffshit-Case.bin"), pattern(100, 1));
    write(dir_path("FFSHIT-CASE.BIN"), pattern(200, 2));

    expected[dir_path("FFSHIT-CASE.BIN")] = file_entry(pattern(200, 2));

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, RemovesFilesAndEmptyDirectories) {
    Tree        expected    = snapshot(*original.filesystem);
    std::string firmware    = firmware_file(expected, GetParam().partition);

    ASSERT_FALSE(firmware.empty());

    original.filesystem->create_directory(dir_path("ffshit-empty"), WRITE_TIME);
    write(dir_path("ffshit-removed.bin"), pattern(4000, 1));

    original.filesystem->remove(dir_path("ffshit-empty"));
    original.filesystem->remove(dir_path("ffshit-removed.bin"));
    original.filesystem->remove(firmware);

    expected.erase(firmware);

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, RefusesToRemoveADirectoryThatIsNotEmpty) {
    std::string dir = firmware_directory(snapshot(*original.filesystem), GetParam().partition);

    ASSERT_FALSE(dir.empty());

    EXPECT_THROW(original.filesystem->remove(dir), FULLFLASH::BaseException);

    expect_unchanged_on_disk();
}

TEST_P(WriteTest, GrowsADirectoryBeyondItsFirstRecord) {
    Tree expected = snapshot(*original.filesystem);

    original.filesystem->create_directory(dir_path("ffshit-many"), WRITE_TIME);
    expected[dir_path("ffshit-many")] = dir_entry();

    for (size_t i = 0; i < 100; ++i) {
        std::string path = dir_path(fmt::format("ffshit-many/file-{:03d}.txt", i));
        std::string data = pattern(i * 13, i);

        write(path, data);
        expected[path] = file_entry(data);
    }

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, ReclaimsTheSpaceOfWhatItReplaced) {
    Tree expected = snapshot(*original.filesystem);

    // More than any of these partitions holds, so the space of the replaced copies has to be
    // reclaimed
    for (size_t i = 0; i < 48; ++i) {
        write(dir_path("ffshit-churn.bin"), pattern(1024 * 1024, i));
    }

    expected[dir_path("ffshit-churn.bin")] = file_entry(pattern(1024 * 1024, 47));

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, RejectsAFileThatDoesNotFit) {
    EXPECT_THROW(write(dir_path("ffshit-huge.bin"), pattern(64 * 1024 * 1024, 1)), FULLFLASH::BaseException);

    expect_unchanged_on_disk();
}

TEST_P(WriteTest, RejectsBadPaths) {
    auto &fs = *original.filesystem;

    std::string firmware = firmware_file(snapshot(fs), GetParam().partition);

    ASSERT_FALSE(firmware.empty());

    EXPECT_THROW(write("FFS_NOPE/ffshit.bin", "x"), FULLFLASH::BaseException);
    EXPECT_THROW(write(dir_path("ffshit-missing/ffshit.bin"), "x"), FULLFLASH::BaseException);
    EXPECT_THROW(write(firmware + "/ffshit.bin", "x"), FULLFLASH::BaseException);
    EXPECT_THROW(write(dir_path(), "x"), FULLFLASH::BaseException);
    EXPECT_THROW(write(GetParam().partition, "x"), FULLFLASH::BaseException);
    EXPECT_THROW(write(dir_path("ffshit\\x.bin"), "x"), FULLFLASH::BaseException);
    EXPECT_THROW(fs.create_directory(dir_path(), WRITE_TIME), FULLFLASH::BaseException);
    EXPECT_THROW(fs.create_directory(firmware, WRITE_TIME), FULLFLASH::BaseException);
    EXPECT_THROW(fs.remove(dir_path("ffshit-missing.bin")), FULLFLASH::BaseException);
    EXPECT_THROW(fs.remove(GetParam().partition), FULLFLASH::BaseException);

    expect_unchanged_on_disk();
}

TEST_P(WriteTest, WritesNamesBeyondAscii) {
    Tree expected = snapshot(*original.filesystem);

    // On SGOLD in the codepage, and in UTF-8 when the codepage lacks a character
    for (const std::string name : { "ffshit-Ärger.bin", "ffshit-файл.bin", "ffshit-中文.bin" }) {
        write(dir_path(name), pattern(100, name.size()));
        expected[dir_path(name)] = file_entry(pattern(100, name.size()));
    }

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, FoldsTheCaseOfWhatTheFirmwareFolds) {
    Tree expected = snapshot(*original.filesystem);

    write(dir_path("ffshit-ärger.bin"), pattern(100, 1));
    write(dir_path("ffshit-Ärger.bin"), pattern(200, 2));

    // SGOLD folds ASCII letters only: both files are there
    if (GetParam().platform == Type::SGOLD) {
        expected[dir_path("ffshit-ärger.bin")] = file_entry(pattern(100, 1));
    }

    expected[dir_path("ffshit-Ärger.bin")] = file_entry(pattern(200, 2));

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

TEST_P(WriteTest, KeepsSgoldNamesInThePhonesCodepage) {
    if (GetParam().platform != Type::SGOLD) {
        GTEST_SKIP() << "only SGOLD names are 8-bit";
    }

    original.filesystem->set_codepage("CP1251");

    write(dir_path("ffshit-файл.bin"), pattern(100, 1));
    // CP1251 has no Ä
    write(dir_path("ffshit-Ärger.bin"), pattern(100, 2));

    Loaded cp1251 = save_and_reload(original, "CP1251");

    EXPECT_NE(find_file(*cp1251.filesystem, dir_path("ffshit-файл.bin")), nullptr);
    EXPECT_NE(find_file(*cp1251.filesystem, dir_path("ffshit-Ärger.bin")), nullptr);

    // The bytes of "файл" in CP1251 are "ôàéë" in CP1252, a name in UTF-8 reads the same in both
    Loaded cp1252 = save_and_reload(original, "CP1252");

    EXPECT_NE(find_file(*cp1252.filesystem, dir_path("ffshit-ôàéë.bin")), nullptr);
    EXPECT_NE(find_file(*cp1252.filesystem, dir_path("ffshit-Ärger.bin")), nullptr);
}

TEST_P(WriteTest, RejectsAnUnknownCodepage) {
    EXPECT_THROW(original.filesystem->set_codepage("NO-SUCH-CODEPAGE"), FULLFLASH::BaseException);
}

TEST_P(WriteTest, ShowsTheChangesBeforeTheyAreSaved) {
    Tree expected = snapshot(*original.filesystem);

    write(dir_path("ffshit-unsaved.bin"), pattern(3000, 1));
    expected[dir_path("ffshit-unsaved.bin")] = file_entry(pattern(3000, 1));

    // In the tree of the filesystem that wrote it, and to a filesystem loaded anew
    expect_tree(snapshot(*original.filesystem), expected);

    auto partitions = original.fullflash->get_partitions();
    auto reloaded   = FULLFLASH::Filesystem::build(partitions->get_fs_platform(), partitions);

    reloaded->load();

    expect_tree(snapshot(*reloaded), expected);
}

TEST_P(WriteTest, WritesIntoEveryPartition) {
    Tree expected = snapshot(*original.filesystem);

    for (const auto &partition : original.filesystem->get_root()->get_subdirs()) {
        std::string path = partition->get_name() + "/ffshit-" + partition->get_name() + ".bin";

        write(path, pattern(10000, 1));
        expected[path] = file_entry(pattern(10000, 1));
    }

    expect_tree(snapshot(*save_and_reload(original).filesystem), expected);
}

INSTANTIATE_TEST_SUITE_P(Phones, WriteTest, ::testing::ValuesIn(PHONES), [](const auto &info) {
    return info.param.name;
});
