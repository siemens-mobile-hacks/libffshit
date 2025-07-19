#include "ffshit/filesystem/platform/sgold.h"
#include "ffshit/filesystem/ex.h"

#include "ffshit/help.h"
#include "ffshit/log/logger.h"

#include <sys/stat.h>

#include <fstream>

// Судя по всему, структура заголовка такая
// Например

// 6C 01 20 01 00 00 21 30 6D 01 00 00 FF FF 6E 01 68 61 74 34 2E 70 6E 67 00

// 6C 01 - ID записи
// 20 01 - ID Родителя (родительский каталог)
// 00 00 21 30 Пока не понятно
// 6D 01 - Вероятно, ID блока с данныем, который идёт хвостом

// 00 00 FF FF - Какие-то аттрибуты если каталог
// 10 00 FF FF - Какие-то аттрибуты если файл

// 6E 01 - Если файл умещается в 1024 байта оно равно FF FF, в противном случае - ID следующего описательного блока
// 68 61 74 34 2E 70 6E 67 00 - Имя файла/каталога

// За загаловком следует блок данных файла

// =============================================================

// Т.е. если файл умещается в 1 блок 1024
// Загаловок
// Данные

// Если файл не умещается в 1024
// Заголовок
// Данные
// Загаловок part
// Данные
// Загаловок part
// Данные
// ....
// пока не влезем

// =============================================================

// Загаловок part - пока не полностью понятно
// 6E 01 20 01 00 00 21 30 6F 01 00 00 6C 01 70 01 
// 6E 01 - ID
// 20 01 - Родитель
// 00 00 21 30 Пока не понятно
// 6F 01 - ID блока с данными
// 6C 01 - ID предыдущего блока
// 70 01 - ID следующего описательного part блока

// Последние 4 байта - ID следующего блока. Если последний - FF FF

// =============================================================

// Например hat4.png
// 6C 01 20 01 00 00 21 30 6D 01 00 00 FF FF 6E 01 68 61 74 34 2E 70 6E 67 00 
// Данные ID 6D 01 - исходя из заголовка
// 6E 01 20 01 00 00 21 30 6F 01 00 00 6C 01 70 01 
// Данные ID 6F 01 - исходя из заголовка
// 70 01 20 01 00 00 21 30 71 01 00 00 6E 01 FF FF
// Данные ID 71 01 - исходя из заголовка


namespace FULLFLASH {
namespace Filesystem {

SGOLD::SGOLD(Partitions::Partitions::Ptr partitions) : partitions(partitions), prototype_6000(false) {
    if (!partitions) {
        throw Exception("SGOLD partitions == nullptr o_O");
    }

    root_dir = Directory::build(ROOT_NAME, "/");
}

void SGOLD::load(bool skip_broken, bool skip_dup, bool dump_data) {
    parse_FIT(skip_broken, skip_dup, dump_data);
}

const Directory::Ptr SGOLD::get_root() const {
    return root_dir;
}

void SGOLD::print_fit_header(const SGOLD::FITHeader &header) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("FIT:");
    Log::Logger::debug("Flags:  {:08X}",  header.flags);
    Log::Logger::debug("ID:     {:d}",    header.id);
    Log::Logger::debug("Size:   {:d}",    header.size);
    Log::Logger::debug("Offset: {:08X}",  header.offset);
}

void SGOLD::print_file_header(const SGOLD::FileHeader &header) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("File:");
    Log::Logger::debug("ID:            {}",      header.id);
    Log::Logger::debug("Parent ID:     {}",      header.parent_id);
    Log::Logger::debug("FAT timestamp: {:04X} {}",  header.fat_timestamp, header.fat_timestamp);
    Log::Logger::debug("Data ID:       {}",      header.data_id);
    Log::Logger::debug("Attributes:    {:04X}",  header.attributes);
    Log::Logger::debug("Next part ID:  {}",      header.next_part);
    Log::Logger::debug("Name:          {}",      header.name);
}

void SGOLD::print_file_part(const SGOLD::FilePart &part) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("File part:");
    Log::Logger::debug("ID:            {}",      part.id);
    Log::Logger::debug("Parent ID:     {}",      part.parent_id);
    Log::Logger::debug("Unknown:       {:04X}",  part.unknown);
    Log::Logger::debug("Data ID:       {}",      part.data_id);
    Log::Logger::debug("Unknown2:      {:04X}",  part.unknown2);
    Log::Logger::debug("Prev ID:       {}",      part.prev_id);
    Log::Logger::debug("Next part ID:  {}",      part.next_part);
}

SGOLD::FileHeader SGOLD::read_file_header(const RawData &data) {
    SGOLD::FileHeader   header;
    size_t              offset = 0;

    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.fat_timestamp), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.data_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.attributes), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
    data.read_string(offset, header.name);

    if (header.name.size() >= 2 && header.name.at(0) == 0x1F) {
        header.name.erase(header.name.begin(), header.name.begin() + 1);
    }
    
    return header;
}

SGOLD::FilePart SGOLD::read_file_part(const RawData &data) {
    SGOLD::FilePart part;
    size_t          offset = 0;

    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.parent_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.unknown), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.data_id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.unknown2), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.prev_id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.next_part), 1);

    return part;
}

void SGOLD::print_data(const FFSBlock &block) {
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

void SGOLD::parse_FIT(bool skip_broken, bool skip_dup, bool dump_data) {
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

        FSBlocksMap ffs_map;

        for (const auto &block : part_blocks) {
            Log::Logger::debug("  Block {:08X} Size: {}, end: {:08X}", block.get_addr(), block.get_size(), block.get_addr() + block.get_size());

            const RawData & block_data = block.get_data();
            size_t          block_size = block.get_size();

            for (ssize_t offset = block_size - 16; offset > 0; offset -= 16) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                print_fit_header(fs_block.header);

                if (fs_block.header.flags == 0xFFFFFFFF) {
                    break;
                }

                if (fs_block.header.flags != 0xFFFFFFC0) {
                    continue;
                }

                fs_block.data = RawData(block_data, fs_block.header.offset, fs_block.header.size);

                if (dump_data) {
                    print_data(fs_block);
                }

                if (ffs_map.count(fs_block.header.id)) {
                    if (skip_dup) {
                        Log::Logger::debug("Duplicate id {}", fs_block.header.id);

                        continue;
                    }

                    throw Exception("Duplicate id {}", fs_block.header.id);
                }

                ffs_map[fs_block.header.id] = fs_block;
            }
        }

        uint16_t root_block_id = 6;

        if (!ffs_map.count(root_block_id)) {
            Log::Logger::warn("{} Root block (ID: 6) not found. Prototype? Trying add 6000", part_name);

            root_block_id += 6000;

            if (!ffs_map.count(root_block_id)) {
                if (skip_broken) {
                    Log::Logger::warn("{} Root block (ID: 6006) not found. Broken filesystem?", part_name);

                    continue;
                } else {
                    throw Exception("{} Root block (ID: 6006) not found. Broken filesystem?", part_name);
                }
            } else {
                prototype_6000 = true;
            }
        }

        try {
            const FFSBlock &    root_block      = ffs_map.at(root_block_id);
            FileHeader          root_header     = read_file_header(root_block.data);
            auto                timestamp       = fat_timestamp_to_unix(root_header.fat_timestamp);

            Directory::Ptr      root            = Directory::build(part_name, ROOT_PATH, timestamp);

            root_dir->add_subdir(root);

            scan(part_name, ffs_map, root, root_header, skip_broken);
        } catch (const FULLFLASH::BaseException &e) {
            if (skip_broken) {
                Log::Logger::warn("Skip. Broken root directory: {}", e.what());
            } else {
                throw;
            }
        }
    }
}

void SGOLD::scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, bool skip_broken, std::string path) {
    if (skip_broken) {
        for (const auto &p_id : recourse_protector) {
            if (header.id == p_id) {
                throw Exception("Directory id already in list");
            }
        }

        recourse_protector.push_back(header.id);
    }

    RawData data;

    try {
        read_full_data(ffs_map, header, data);
    } catch (const FULLFLASH::BaseException &e) {
        if (skip_broken) {
            Log::Logger::warn("Skip. Broken directory: {}", e.what());

            recourse_protector.pop_back();

            return;
        } else {
            throw;
        }
    }

    size_t                  offset  = 0;
    std::vector<uint16_t>   id_list;

    while (offset < data.get_size()) {
        uint32_t raw;

        data.read<uint32_t>(offset, reinterpret_cast<char *>(&raw), 1);

        uint16_t    id = raw & 0xFFFF;
        uint16_t    unk = (raw >> 16) & 0xFFFF;

        if (id == 0xFFFF) {
            continue;
        }

        if (id == 0) {
            continue;
        }

        if (prototype_6000) {
            id += 6000;
        }

        id_list.push_back(id);
    }

    for (const auto &id : id_list) {
        if (!ffs_map.count(id)) {
            if (skip_broken) {
                Log::Logger::warn("Skip. FFS Block ID {} not found", id);

                continue;
            } else {
                throw Exception("FFS Block ID: {} not found", id);
            }
        }

        try {
            const FFSBlock &tmp         = ffs_map.at(id);
            FileHeader      file_header = read_file_header(tmp.data);
            auto            timestamp   = fat_timestamp_to_unix(file_header.fat_timestamp);

            Log::Logger::info("Processing ID: {:5d}, Path: {}{}{}", id, block_name, path, file_header.name);

            if (file_header.attributes & 0x10) {
                Directory::Ptr dir_next = Directory::build(file_header.name, block_name + path, timestamp);

                dir->add_subdir(dir_next);

                scan(block_name, ffs_map, dir_next, file_header, skip_broken, path + file_header.name + "/");
            } else {
                RawData file_data;

                if (ffs_map.count(file_header.data_id)) {
                    read_full_data(ffs_map, file_header, file_data);
                }
                                
                File::Ptr file = File::build(file_header.name, block_name + path, timestamp, file_data);

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

void SGOLD::read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id) {
    if (!ffs_map.count(next_id)) {
        throw Exception("Reading part. Couldn't find block with id: {}", next_id);
    }

    const FFSBlock &block      = ffs_map.at(next_id);

    FilePart        part       = read_file_part(block.data);

    print_file_part(part);

    if (prototype_6000) {
        part.data_id += 6000;
    }

    if (!ffs_map.count(part.data_id)) {
        throw Exception("Reading part data. Couldn't find block with id: {}", part.data_id);
    }

    FFSBlock &      block_data = ffs_map.at(part.data_id);

    data.add(block_data.data);

    if (part.next_part != 0xFFFF) {
        read_recurse(ffs_map, data, part.next_part);
    }
}

void SGOLD::read_full_data(FSBlocksMap &ffs_map, const FileHeader &header, RawData &file_data) {
    print_file_header(header);

    uint16_t data_id = header.data_id;

    if (prototype_6000) {
        data_id += 6000;
    }

    if (!ffs_map.count(data_id)) {
        throw Exception("Reading file data. Couldn't find block with id: {}", data_id);
    }

    const FFSBlock &block = ffs_map.at(data_id);

    file_data.add(block.data);

    if (header.next_part != 0xFFFF) {
        read_recurse(ffs_map, file_data, header.next_part);
    }
}

};
};
