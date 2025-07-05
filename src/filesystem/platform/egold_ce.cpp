#include "ffshit/filesystem/platform/egold_ce.h"
#include "ffshit/filesystem/ex.h"

#include "ffshit/help.h"
#include "ffshit/log/logger.h"

namespace FULLFLASH {
namespace Filesystem {

EGOLD_CE::EGOLD_CE(Partitions::Partitions::Ptr partitions) : partitions(partitions) { }

void EGOLD_CE::load(bool skip_broken) {
    parse_FIT(skip_broken);
}

const FSMap &EGOLD_CE::get_filesystem_map() const {
    return fs_map;
}

void EGOLD_CE::print_block_header(const FFSBlock &block) {
    Log::Logger::debug("    ==== Offset: {:08X} ====", block.addr);
    Log::Logger::debug("    Marker1:  {:04X}", block.header.marker1);
    Log::Logger::debug("    Size:     {:04X}", block.header.size);
    Log::Logger::debug("    Offset:   {:08X} {:08X}", block.header.offset, block.addr_start);
    Log::Logger::debug("    Block ID: {:04X}", block.header.block_id);
    Log::Logger::debug("    Marker2:  {:04X}", block.header.marker2);
}

void EGOLD_CE::print_file_header(const FFSFile &file) {
    Log::Logger::debug("    ID:           {:04X} {}", file.header.id, file.header.id);
    Log::Logger::debug("    Parent:       {:04X} {}", file.header.parent_id, file.header.parent_id);
    Log::Logger::debug("    Timestamp:    {:08X}", file.header.fat_timestamp);
    Log::Logger::debug("    Flags:        {:04X}", file.header.flags);
    Log::Logger::debug("    Data ID:      {:04X} {:04X}", file.header.data_id, file.header.data_id + ID_ADD);
    Log::Logger::debug("    Unk4:         {:04X}", file.header.unk4);
    Log::Logger::debug("    Next part ID: {:04X}", file.header.next_part_id);

    if (file.header.name.size()) {
        Log::Logger::debug("    Name:      {}", file.header.name);
    }
}

void EGOLD_CE::print_data(const FFSBlock &block) {
    std::string data_print;

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

void EGOLD_CE::parse_FIT(bool skip_broken) {
    const auto &part_map = partitions->get_partitions();

    for (const auto &pair : part_map) {
        const std::string & part_name   = pair.first;
        const auto &        part_info   = pair.second;
        const auto &        part_blocks = part_info.get_blocks();

        if (part_name.find("FFS") != std::string::npos) {
            Log::Logger::debug("Partition: {}, Blocks {}", part_name, part_blocks.size());
        } else {
            continue;
        }

        std::map<uint16_t, FFSBlock> ffs_blocks;
        std::map<uint16_t, FFSFile>  ffs_files;

        Log::Logger::debug("Collecting FFS blocks");

        for (const auto &block : part_blocks) {
            const RawData & block_data = block.get_data();
            size_t          block_size = block.get_size();

            uint32_t block_addr = block.get_addr();

            Log::Logger::debug("  Block: {:08X} ====", block_addr);

            for (ssize_t offset = block_size - 12; offset > 0; offset -= 12) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;
                
                fs_block.addr_start = block_addr;
                fs_block.addr       = block_addr + offset;

                block_data.read<uint16_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.marker1), 1);
                block_data.read<uint16_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);
                block_data.read<uint16_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.block_id), 1);
                block_data.read<uint16_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.marker2), 1);

                if (fs_block.header.marker1 == 0xFFFF) {
                    break;
                }

                // if ((fs_block.header.marker1 != 0x00FC && fs_block.header.marker2 != 0xFC00) &&
                //     (fs_block.header.marker1 != 0x00F0 && fs_block.header.marker2 != 0xF000)) {
                //     break;
                // }

                if (((fs_block.header.marker1 & 0x00FF) == 0x00F0)  && ((fs_block.header.marker2 & 0xFF00) == 0xF000)) {
                    // print_block_header(fs_block);
                    // print_data(fs_block);

                    continue;
                }
                
                if ((fs_block.header.marker1 & 0x00FF) != 0xFC) {
                    continue;
                }

                print_block_header(fs_block);
                print_data(fs_block);

                size_t addr_mask    = block.get_size() - 1;
                size_t addr_data    = fs_block.header.offset & addr_mask;

                // Log::Logger::debug("addr mask: {:08X} {:08X} {:08X}", block.get_addr(), fs_block.header.offset, addr_mask);

                fs_block.data = RawData(block_data, addr_data, fs_block.header.size);

                if (ffs_blocks.count(fs_block.header.block_id)) {
                    const auto &exists_block = ffs_blocks.at(fs_block.header.block_id);

                    Log::Logger::debug("  Exists block:");
                    print_block_header(exists_block);
                    print_data(exists_block);

                    if (exists_block.header.size == fs_block.header.size && exists_block.header.offset == fs_block.header.offset) {
                        Log::Logger::warn("Block already exists. {:04} {}", fs_block.header.block_id, fs_block.header.block_id);
                    } else {
                        throw Exception("Block already exists. {:04} {}",fs_block.header.block_id, fs_block.header.block_id);
                    }
                }

                ffs_blocks[fs_block.header.block_id] = fs_block;
            }
        }

        Log::Logger::debug("Collecting FFS files");

        for (const auto &pair : ffs_blocks) {
            auto        fs_block_id = pair.first;
            const auto &fs_block    = pair.second;

            bool is_header = !(fs_block.header.block_id & 1);

            print_block_header(fs_block);
            print_data(fs_block);

            if (!is_header) {
                continue;
            }

            if (fs_block.header.size < 0x10) {
                continue;
            }

            FFSFile file;
            size_t  oofs = 0;

            file.block = &fs_block;

            fs_block.data.read<uint16_t>(oofs, reinterpret_cast<char *>(&file.header.id), 1);
            fs_block.data.read<uint16_t>(oofs, reinterpret_cast<char *>(&file.header.parent_id), 1);
            fs_block.data.read<uint32_t>(oofs, reinterpret_cast<char *>(&file.header.fat_timestamp), 1);
            fs_block.data.read<uint16_t>(oofs, reinterpret_cast<char *>(&file.header.data_id), 1);
            fs_block.data.read<uint16_t>(oofs, reinterpret_cast<char *>(&file.header.flags), 1);
            fs_block.data.read<uint16_t>(oofs, reinterpret_cast<char *>(&file.header.unk4), 1);
            fs_block.data.read<uint16_t>(oofs, reinterpret_cast<char *>(&file.header.next_part_id), 1);

            if (fs_block.header.size > 0x10) {
                // old/new egold

                if (fs_block.header.size > 0x14) {
                    uint32_t wtf;
                    fs_block.data.read<uint32_t>(oofs, reinterpret_cast<char *>(&wtf), 1);

                    if (wtf != 0xFFFFFFFF) {
                        oofs -= 4;
                    }
                }

                fs_block.data.read_string(oofs, file.header.name);

                if (file.header.name.size() >= 2 && file.header.name.at(0) == 0x1F) {
                    file.header.name.erase(file.header.name.begin(), file.header.name.begin() + 1);
                }
            }

            print_file_header(file);

            if (ffs_files.count(file.header.id)) {
                throw Exception("File id {:04X} already exists in map", file.header.id);
            }

            ffs_files[file.header.id] = file;
        }

        if (!ffs_files.count(6)) {
            throw Exception("root block not found. Empty filesystem?");
        }

        const auto &        root_block = ffs_files.at(6);
        Directory::Ptr      root = Directory::build(part_name, "/");

        scan(part_name, ffs_blocks, ffs_files, root_block, root, skip_broken);

        fs_map[part_name] = root;
    }
}

void EGOLD_CE::scan(const std::string &part_name, const FFSBlocksMap &ffs_blocks, const FFSFilesMap &ffs_files, const FFSFile &file, Directory::Ptr dir, bool skip_broken, std::string path) {
    RawData dir_data;

    try {
        read_full(ffs_blocks, ffs_files, file, dir_data);
    } catch (const Exception &e) {
        if (skip_broken) {
            Log::Logger::warn("Skip. Broken directory: {}", e.what());

            return;
        } else {
            throw;
        }
    }


    std::vector<uint16_t>   dir_id_list;
    
    for (size_t i = 0; i < dir_data.get_size(); i += 2) {
        uint16_t id;

        dir_data.read<uint16_t>(i, reinterpret_cast<char *>(&id), 1);

        if (id == 0x0000) {
            continue;
        }
        
        if (id == 0xFFFF) {
            continue;
        }

        dir_id_list.push_back(id);
    }

    for (const auto &id : dir_id_list) {
        if (!ffs_files.count(id)) {
            if (skip_broken) {
                Log::Logger::warn("Skip. File record ID {} not found", id);
            } else {
                throw Exception("File record ID {} not found", id);
            }
        }

        const auto &file        = ffs_files.at(id);
        auto        timestamp   = fat_timestamp_to_unix(file.header.fat_timestamp);

        Log::Logger::info("Found ID: {:5d} {:5d}, Path: {}{}{}", file.block->header.block_id, file.header.id, part_name, path, file.header.name);

        if (file.header.flags & 0x10) {
            Directory::Ptr dir_next = Directory::build(file.header.name, part_name + path, timestamp);

            dir->add_subdir(dir_next);

            scan(part_name, ffs_blocks, ffs_files, file, dir_next, skip_broken, path + file.header.name + "/");
        } else {
            try {
                RawData file_data;

                read_full(ffs_blocks, ffs_files, file, file_data);

                File::Ptr file_ = File::build(file.header.name, part_name + path, timestamp, file_data);

                dir->add_file(file_);
            } catch (const Exception &e) {
                if (skip_broken) {
                    Log::Logger::info("Skip. Broken file: {}", e.what());
                } else {
                    throw;
                }
            }

        }
    }
}

void EGOLD_CE::read_full(const FFSBlocksMap &ffs_map, const FFSFilesMap &ffs_files, const FFSFile &file, RawData &data) {
    uint32_t data_block_id = file.header.data_id + ID_ADD;

    if (!ffs_map.count(data_block_id)) {
        return;
    }

    const auto &data_block = ffs_map.at(data_block_id);

    data.add(data_block.data);

    if (file.header.next_part_id == 0xFFFF) {
        return;
    }

    read_recurse(ffs_map, ffs_files, file, data, file.header.next_part_id);
}


void EGOLD_CE::read_recurse(const FFSBlocksMap &ffs_map, const FFSFilesMap &ffs_files, const FFSFile &file, RawData &data, uint16_t next_file_id) {
    if (!ffs_files.count(next_file_id)) {
        throw Exception("read_recurse() Next file id {} not found", next_file_id);
    }

    const auto &next_part = ffs_files.at(next_file_id);
    uint32_t    data_id = next_part.block->header.block_id + 1;

    if (!ffs_map.count(data_id)) {
        throw Exception("read_recurse() Next data id {} not found", data_id);
    }

    const auto &data_block = ffs_map.at(data_id);

    data.add(data_block.data);

    if (next_part.header.next_part_id == 0xFFFF) {
        return;
    }

    read_recurse(ffs_map, ffs_files, file, data, next_part.header.next_part_id);
}

};
};
