#include "ffshit/filesystem/platform/sgold2_elka.h"

#include "ffshit/filesystem/ex.h"
#include "ffshit/log/logger.h"

#include <iconv.h>

namespace FULLFLASH {
namespace Filesystem {

SGOLD2_ELKA::SGOLD2_ELKA(Partitions::Partitions::Ptr partitions) : partitions(partitions) {
    if (!partitions) {
        throw Exception("SGOLD2_ELKA partitions == nullptr o_O");
    }

    root_dir = Directory::build(ROOT_NAME, "/");
}

void SGOLD2_ELKA::load(bool skip_broken, bool skip_dup, std::vector<std::string> parts_to_extract) {
    Log::Logger::info("Loading filesystem");

    auto start_time = std::chrono::high_resolution_clock::now();

    parse_FIT(skip_broken, skip_dup, parts_to_extract);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto diff_time = end_time - start_time;

    Log::Logger::info("Done in {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(diff_time).count());
}

const Directory::Ptr SGOLD2_ELKA::get_root() const {
    return root_dir;
}

void SGOLD2_ELKA::print_fit_header(const SGOLD2_ELKA::FITHeader &header) {
    if (!verbose_headers) {
        return;
    }

    Log::Logger::debug("    ===========================");
    Log::Logger::debug("    FIT:");
    Log::Logger::debug("      Flags:      {:08X}",      header.flags);
    Log::Logger::debug("      ID:         {:08X} {:d}", header.id, header.id);
    Log::Logger::debug("      Size:       {:08X} {:d}", header.size, header.size);
    Log::Logger::debug("      Offset:     {:08X}",      header.offset);
}

void SGOLD2_ELKA::print_dir_header(const DirHeader &header) {
    if (!verbose_headers) {
        return;
    }

    Log::Logger::debug("===========================");
    Log::Logger::debug("Dir header:");
    Log::Logger::debug("  ID:           {:04X} {}",     header.id, header.id);
    Log::Logger::debug("  Unknown1:     {:04X} {}",     header.unknown1, header.unknown1);
    Log::Logger::debug("  Unknown2:     {:04X} {}",     header.unknown2, header.unknown2);
    Log::Logger::debug("  Unknown3:     {:04X} {}",     header.unknown3, header.unknown3);
}

void SGOLD2_ELKA::print_file_header(const SGOLD2_ELKA::FileHeader &header) {
    if (!verbose_headers) {
        return;
    }

    Log::Logger::debug("===========================");
    Log::Logger::debug("File:");
    Log::Logger::debug("  ID:            {:04X} {}",     header.id, header.id);
    Log::Logger::debug("  Unknown1:      {:04X} {}",     header.unknown1, header.unknown1);
    Log::Logger::debug("  Parent ID:     {:04X} {}",     header.parent_id, header.parent_id);
    Log::Logger::debug("  Next part ID:  {:04X} {}",     header.next_part, header.next_part);
    Log::Logger::debug("  Size:          {:08X} {}",     header.size, header.size);
    Log::Logger::debug("  FAT timestamp: {:08X} {}",     header.fat_timestamp, header.fat_timestamp);
    Log::Logger::debug("  Attributes:    {:04X} {}",     header.attributes, header.attributes);
    Log::Logger::debug("  Unknown7:      {:04X} {}",     header.unknown7, header.unknown7);
    Log::Logger::debug("  Name:          {}",            header.name);
}

void SGOLD2_ELKA::print_file_part(const FilePart &part) {
    if (!verbose_headers) {
        return;
    }

    Log::Logger::debug("===========================");
    Log::Logger::debug("Part:");
    Log::Logger::debug("  ID:             {:04X} {}", part.id, part.id);
    Log::Logger::debug("  Parent ID:      {:04X} {}", part.parent_id, part.parent_id);
    Log::Logger::debug("  Next part ID:   {:04X} {}", part.next_part, part.next_part);
}

SGOLD2_ELKA::FileHeader SGOLD2_ELKA::read_file_header(const FFSBlock &block, bool skip_broken) {
    const RawData &             header_data = block.data;
    SGOLD2_ELKA::FileHeader     header;
    size_t                      offset = 0;
    static size_t               borken_names = 0;

    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown1), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.size), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.fat_timestamp), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.attributes), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown7), 1);

    // ======================================================================

    offset = 28;

    size_t  str_size    = header_data.get_size() - 28;
    size_t  from_n      = str_size;
    char *  from        = new char[str_size];

    size_t  to_n        = str_size;
    char *  to          = new char[to_n * 2];

    header_data.read<char>(offset, from, from_n);

    iconv_t iccd = iconv_open("UTF-8", "UTF-16LE");

    if (iccd == ((iconv_t)-1)) {
        throw Exception("iconv_open(): {}", strerror(errno));
    }

#ifdef __MINGW32__
    const char *inptr = from;
    char *      ouptr = to;
#else
    char *  inptr = from;
    char *  ouptr = to;
#endif

    int r = iconv(iccd, &inptr, &from_n, &ouptr, &to_n);

    if (r == -1) {
        iconv_close(iccd);

        delete []from;
        delete []to;

        if (skip_broken) {
            throw Exception("iconv(): {}", strerror(errno));
        } else {
            std::string hex;

            for (size_t i = 0; i < str_size; ++i) {
                hex += fmt::format("{:02X} ", static_cast<uint8_t>(from[i]));
            }

            header.name = fmt::format("broken_name_{}", borken_names++);

            Log::Logger::warn("Broken name: {} -> {}", hex, header.name);
        }
    } else {
        to[str_size - to_n] = 0x00;

        header.name = std::string(to);

        iconv_close(iccd);

        delete []from;
        delete []to;
    }

    return header;
}

SGOLD2_ELKA::FilePart SGOLD2_ELKA::read_file_part(const RawData &data) {
    FilePart    part;
    size_t      offset = 0;

    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.parent_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.next_part), 1);

    return part;
}

void SGOLD2_ELKA::print_data(const FFSBlock &block) {
    if (!verbose_data) {
        return;
    }

    std::string data_print;

    const auto &block_data = block.data;

    for (size_t i = 0; i < block.data.get_size(); ++i) {
        char *c = block.data.get_data().get() + i;

        data_print += fmt::format("{:02X} ", static_cast<uint8_t>(*c));

        if ((!((i + 1) % 16)) ||
            (block.data.get_size() < 16 && i + 1 == block.data.get_size())) {
            Log::Logger::debug("    {}", data_print);

            data_print.clear();
        }
    }

    if ((block.data.get_size() % 16)) {
        Log::Logger::debug("    {}", data_print);
    }
}

void SGOLD2_ELKA::parse_FIT(bool skip_broken, bool skip_dup, std::vector<std::string> parts_to_extract) {
    const auto &part_map = partitions->get_partitions();

    static auto check_end = [](const FITHeader &header) -> bool {
        return  header.flags    == 0xFFFFFFFF &&
                header.id       == 0xFFFFFFFF &&
                header.size     == 0xFFFFFFFF &&
                header.offset   == 0xFFFFFFFF;
    };

    static auto calc_aligned_size = [](size_t size) -> size_t {
        return ceil((size / 16.0) + 1) * 32.0;
    };

    for (const auto &pair : part_map) {
        const std::string & part_name   = pair.first;
        const auto &        part_info   = pair.second;
        const auto &        part_blocks = part_info.get_blocks();

        if (part_name.find("FFS") != std::string::npos) {
            if (parts_to_extract.size() != 0) {
                bool is_extract = false;

                for (const auto &part_to_extract : parts_to_extract) {
                    if (part_name == part_to_extract) {
                        is_extract = true;

                        break;
                    }
                }

                if (!is_extract) {
                    Log::Logger::warn("Partition {} excluded", part_name);

                    continue;
                }
            }

            Log::Logger::debug("Partition: {}, Blocks {}", part_name, part_blocks.size());
        } else {
            continue;
        }

        FSBlocksMap ffs_map;

        for (const auto &block : part_blocks) {
            const auto &block_header = block.get_header();

             Log::Logger::debug("  {} Block {:08X} {:08X} {:08X} {:08x} Size: {}",
                part_name,
                block.get_addr(),
                block_header.unknown_1,
                block_header.unknown_2,
                block_header.unknown_3,
                block.get_size());

            const RawData & block_data  = block.get_data();
            uint32_t        block_size  = block.get_size();

            uint32_t        offset      = block_size - 64;

            while (offset > 0) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                if (check_end(fs_block.header)) {
                    Log::Logger::debug("{} {:08X}: Flags: {:08X} ID: {:08X} Size: {:08X} Offset: {:08X} - End", 
                        part_name, 
                        block.get_addr() + offset, 
                        fs_block.header.flags, 
                        fs_block.header.id,
                        fs_block.header.size, 
                        fs_block.header.offset);

                    break;
                }

                uint32_t    ff_boffset      = block.get_addr();

                size_t      block_header_size      = fs_block.header.size;
                size_t      block_header_size_hi   = fs_block.header.size & 0x1C00;
                size_t      block_header_size_lo   = fs_block.header.size & 0x3FF;

                bool        read_from_table             = (block_header_size <= 0x200);
                bool        read_from_table_and_offset  = (block_header_size_hi && block_header_size_lo <= 0x200 && block_header_size_lo > 0);
                bool        read_from_offset            = !read_from_table_and_offset && !read_from_table;

                if (read_from_table) {
                    uint32_t size_data = calc_aligned_size(fs_block.header.size);

                    if (fs_block.header.flags == 0xFFFFFFC0) {
                        size_t read_size = fs_block.header.size;

                        fs_block.data = block_data.read_aligned(offset, read_size);
                    }

                    Log::Logger::debug("{} T  {:08X}: Flags: {:08X} ID: {:08X} Size: {:08X} Offset: {:08X} - {:04X}", 
                        part_name, 
                        ff_boffset + offset, 
                        fs_block.header.flags, 
                        fs_block.header.id, 
                        fs_block.header.size, 
                        fs_block.header.offset, 
                        size_data);

                    offset -= size_data;
                }

                if (read_from_table_and_offset) {
                    size_t read_size_block  = fs_block.header.size;
                    size_t read_size        = fs_block.header.size & 0x3FF;

                    if (fs_block.header.flags == 0xFFFFFFC0) {
                        fs_block.data = RawData(this->partitions->get_data(), ff_boffset + fs_block.header.offset, fs_block.header.size & 0xC00);

                        // ===============================

                        auto data_nonaligned = block_data.read_aligned(offset, read_size);

                        fs_block.data.add(data_nonaligned);
                    }

                    size_t size_data = calc_aligned_size(read_size);

                    Log::Logger::debug("{} TO {:08X}: Flags: {:08X} ID: {:08X} Size: {:08X} Offset: {:08X} - {:04X}", 
                        part_name, 
                        block.get_addr() + offset, 
                        fs_block.header.flags, 
                        fs_block.header.id, 
                        fs_block.header.size, 
                        fs_block.header.offset, 
                        size_data);


                    offset -= size_data;
                }

                if (read_from_offset) {
                    if (fs_block.header.flags == 0xFFFFFFC0) {
                        size_t read_addr = ff_boffset + fs_block.header.offset;
                        size_t read_size = fs_block.header.size;

                        fs_block.data = RawData(this->partitions->get_data(), read_addr, read_size);
                    }

                    Log::Logger::debug("{} O  {:08X}: Flags: {:08X} ID: {:08X} Size: {:08X} Offset: {:08X} - {:04X}", 
                        part_name, 
                        block.get_addr() + offset, 
                        fs_block.header.flags, 
                        fs_block.header.id, 
                        fs_block.header.size, 
                        fs_block.header.offset, 
                        32);

                    offset -= 32;
                }

                if (fs_block.header.flags != 0xFFFFFFC0 && fs_block.header.flags != 0xFFFFFF00 && fs_block.header.flags != 0xFFFFFFF0) {
                    throw Exception("fs_block.header.flags == {:08X}", fs_block.header.flags);
                }

                if (fs_block.header.flags != 0xFFFFFFC0) {
                    continue;
                }

                print_data(fs_block);

                if (ffs_map.count(fs_block.header.id)) {
                    if (skip_dup) {
                        Log::Logger::warn("Duplicate id {}", fs_block.header.id);

                        continue;
                    }

                    throw Exception("Duplicate id {}", fs_block.header.id);
                }

                ffs_map[fs_block.header.id] = fs_block;
            }
        }

        // continue;

        if (!ffs_map.count(10)) {
            if (skip_broken) {
                Log::Logger::warn("{} Root block (ID: 10) not found. Broken filesystem?", part_name);

                continue;
            } else {
                throw Exception("{} Root block (ID: 10) not found. Broken filesystem?", part_name);
            }
        }

        try {
            const FFSBlock &    root_block      = ffs_map.at(10);
            FileHeader          root_header     = read_file_header(root_block);
            auto                timestamp       = fat_timestamp_to_unix(root_header.fat_timestamp);
            Attributes          attributes(root_header.attributes);

            Directory::Ptr      root            = Directory::build(part_name, ROOT_PATH, attributes, timestamp);

            print_file_header(root_header);

            scan(part_name, ffs_map, root, root_header, skip_broken);

            root_dir->add_subdir(root);
        } catch (const FULLFLASH::BaseException &e) {
            if (skip_broken) {
                Log::Logger::warn("{} Skip. Broken root directory: {}", part_name, e.what());
            } else {
                throw;
            }
        }
    }
}

void SGOLD2_ELKA::read_full_data(FSBlocksMap &ffs_map, const FileHeader &header, RawData &file_data) {
    uint32_t data_id = header.id + 1;

    if (!ffs_map.count(data_id)) {
        throw Exception("Reading file data. Couldn't find block with id: {}", data_id);
    }

    const FFSBlock &block = ffs_map.at(data_id);

    file_data.add(block.data);

    if (header.next_part != 0xFFFFFFFF) {
        read_recurse(ffs_map, file_data, header.next_part);
    }
}

void SGOLD2_ELKA::read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id) {
    if (!ffs_map.count(next_id)) {
        throw Exception("Reading part. Couldn't find block with id: {}", next_id);
    }

    const FFSBlock &block      = ffs_map.at(next_id);
    FilePart        part       = read_file_part(block.data);
    uint16_t        data_id    = part.id + 1;

    print_file_part(part);

    if (!ffs_map.count(data_id)) {
        throw Exception("Reading part data. Couldn't find block with id: {}", data_id);
    }

    const FFSBlock & block_data = ffs_map.at(data_id);

    data.add(block_data.data);

    if (part.next_part != 0xFFFFFFFF) {
        read_recurse(ffs_map, data, part.next_part);
    }
}

void SGOLD2_ELKA::scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, bool skip_broken, std::string path) {
    RawData dir_data;

    if (skip_broken) {
        for (const auto &p_id : recourse_protector) {
            if (header.id == p_id) {
                throw Exception("Directory id already in list");
            }
        }

        recourse_protector.push_back(header.id);
    }

    try {
        read_full_data(ffs_map, header, dir_data);
    } catch (const FULLFLASH::BaseException &e) {
        if (skip_broken) {
            Log::Logger::warn("Skip. Broken directory: {}", e.what());

            recourse_protector.pop_back();

            return;
        } else {
            throw;
        }
    }

    DirList dir_list;

    size_t offset = 0;

    while (offset < dir_data.get_size()) {
        DirHeader dir_header;

        dir_data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.id), 1);
        dir_data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.unknown1), 1);
        dir_data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.unknown2), 1);
        dir_data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.unknown3), 1);

        if (dir_header.id == 0xFFFF) {
            continue;
        }

        if (dir_header.id == 0) {
            continue;
        }

        if (dir_header.unknown3 != 0xFFFF) {
            continue;
        }

        dir_list.push_back(dir_header);
    }

    // ========================================

    for (const auto &dir_info : dir_list) {
         if (!ffs_map.count(dir_info.id)) {
            throw Exception("scan() ID {} not found in ffs_map", dir_info.id);
        }

        try {
            const auto &        file_block  = ffs_map.at(dir_info.id);
            FileHeader          file_header = read_file_header(file_block);;
            TimePoint           timestamp   = fat_timestamp_to_unix(file_header.fat_timestamp);
            Attributes          attributes(file_header.attributes);

            print_file_header(file_header);

            if (verbose_processing) {
                Log::Logger::info("Processing ID: {:5d}, Path: {}{}", dir_info.id, block_name, path + file_header.name);
            }

            if (attributes.is_directory()) {
                Directory::Ptr dir_next = Directory::build(file_header.name, block_name + path, attributes, timestamp);

                dir->add_subdir(dir_next);

                scan(block_name, ffs_map, dir_next, file_header, skip_broken, path + file_header.name + "/");
            } else {
                RawData     file_data;
                uint32_t    data_id = file_header.id + 1;

                // Для диска Cache блока может и не быть
                if (ffs_map.count(data_id)) {
                    if (file_header.size != 0 ) {
                        file_data.reserve(file_header.size);
                    }

                    read_full_data(ffs_map, file_header, file_data);
                }

                File::Ptr file = File::build(file_header.name, block_name + path, file_data, attributes, timestamp);

                dir->add_file(file);
            }
        } catch (const FULLFLASH::BaseException &e) {
            if (skip_broken) {
                Log::Logger::warn("Skip. Broken file/directory: {}", e.what());
            } else {
                throw;
            }
        }
    }

    if (skip_broken) {
        recourse_protector.pop_back();
    }
}

};
};
