#include "ffshit/filesystem/newsgold.h"
#include "ffshit/filesystem/ex.h"
#include "ffshit/filesystem/extract.h"

#include "ffshit/help.h"
#include "ffshit/system.h"
#include "ffshit/log/logger.h"

#include <fstream>
#include <iostream>
#include <algorithm>

#include <spdlog/spdlog.h>

#include <iconv.h>

namespace FULLFLASH {
namespace Filesystem {

NewSGOLD::NewSGOLD(Blocks &blocks) : blocks(blocks) { }

void NewSGOLD::load() {
    parse_FIT();
}

void NewSGOLD::extract(std::string path, bool overwrite) {
    FULLFLASH::Filesystem::extract(path, overwrite, [&](std::string dst_path) {
        for (const auto &fs : fs_map) {
            std::string fs_name = fs.first;
            auto root           = fs.second;

            Log::Logger::info("Extracting {}", fs_name);

            unpack(root, dst_path + "/" + fs_name);
        };
    });
}

void NewSGOLD::print_fit_header(const NewSGOLD::FITHeader &header) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("FIT:");
    Log::Logger::debug("Flags:      {:08X}",  header.flags);
    Log::Logger::debug("ID:         {:d}",    header.id);
    Log::Logger::debug("Size:       {:d}",    header.size);
    Log::Logger::debug("Offset:     {:04X}",  header.offset);
}

void NewSGOLD::print_file_header(const NewSGOLD::FileHeader &header) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("File:");
    Log::Logger::debug("ID:           {}",      header.id);
    Log::Logger::debug("Parent ID:    {}",      header.parent_id);
    Log::Logger::debug("Next part ID: {}",      header.next_part);
    Log::Logger::debug("Unknown2:     {:04X} {}",      header.unknown2, header.unknown2);
    Log::Logger::debug("Unknown3:     {:04X} {}",      header.unknown3, header.unknown3);
    Log::Logger::debug("Unknown4:     {:04X} {}",      header.unknown4, header.unknown4);
    Log::Logger::debug("Unknown5:     {:04X} {}",      header.unknown5, header.unknown5);
    Log::Logger::debug("Unknown6:     {:04X} {}",      header.unknown6, header.unknown6);
    Log::Logger::debug("Unknown7:     {:04X} {}",      header.unknown7, header.unknown7);
    Log::Logger::debug("Name:         {}",      header.name);
}

void NewSGOLD::print_file_part(const FilePart &part) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("Part:");
    Log::Logger::debug("ID:             {}", part.id);
    Log::Logger::debug("Parent ID:      {}", part.parent_id);
    Log::Logger::debug("Next part ID:   {}", part.next_part);
}

NewSGOLD::FileHeader NewSGOLD::read_file_header(const RawData &data) {
    FileHeader  header;
    size_t      offset = 0;

    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown1), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown2), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown3), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown4), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown5), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown6), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown7), 1);

    iconv_t iccd = iconv_open("UTF-8", "UTF-16LE");

    if (iccd == ((iconv_t)-1)) {
        throw Exception("iconv_open(): {}", strerror(errno));
    }

    offset = 28;

    size_t  str_size    = data.get_size() - 28;

    size_t  from_size   = str_size;
    char *  from        = new char[str_size];

    size_t  to_size     = str_size;
    char *  to          = new char[to_size];

    data.read<char>(offset, from, from_size);

    char *inptr = from;
    char *ouptr = to;

    int r = iconv(iccd, &inptr, &from_size, &ouptr, &to_size);

    if (r == -1) {
        delete[] from;
        delete[] to;

        throw Exception("iconv(): {}", strerror(errno));
    }

    to[str_size - to_size] = 0x00;

    header.name = std::string(to);

    delete []from;
    delete []to;

    return header;
}

NewSGOLD::FilePart NewSGOLD::read_file_part(const RawData &data)  {
    FilePart    part;
    size_t      offset = 0;

    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.parent_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.next_part), 1);

    return part;
}

void NewSGOLD::parse_FIT() {
    Blocks::Map &bl = blocks.get_blocks();

    for (const auto &block : blocks.get_blocks()) {
        const auto &ffs_block_name  = block.first;
        const auto &ffs             = block.second;

        FSBlocksMap ffs_map;

        if (ffs_block_name.find("FFS") != std::string::npos) {
            Log::Logger::debug("{} Blocks: {}", ffs_block_name, ffs.size());
        } else {
            continue;
        }

        for (auto &block : ffs) {
            Log::Logger::debug("  Block {:08X} Size: {}", block.offset, block.data.get_size());

            const RawData & block_data = block.data;
            size_t          block_size = block_data.get_size();

            for (ssize_t offset = block_size - 16; offset > 0; offset -= 16) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                if (fs_block.header.flags == 0xFFFFFFFF) {
                    break;
                }

                if (fs_block.header.flags == 0xFFFFFF00) {
                    continue;
                }
                
                print_fit_header(fs_block.header);

                std::string data_print;

                for (size_t i = 0; i < 16; ++i) {
                    char *c = block_data.get_data().get() + offset + i;
                    
                    data_print += fmt::format("{:02X} ", (unsigned char) *c);

                    if ((i + 1) % 4 == 0) {
                        Log::Logger::debug(data_print);

                        data_print.clear();
                    }
                }

                fs_block.data = RawData(block_data, fs_block.header.offset, fs_block.header.size);

                if (ffs_map.count(fs_block.header.id)) {
                    throw Exception("Duplicate id {}", fs_block.header.id);
                }

                ffs_map[fs_block.header.id] = fs_block;

                // print_fit_header(fs_block.header);

                // Log::Logger::info("Data:\n");

                // for (size_t i = 0; i < fs_block.data.get_size(); ++i) {
                //     Log::Logger::info("{:02X} ", (unsigned char) *(fs_block.data.get_data().get() + i));

                //     if ((i + 1) % 32 == 0) {
                //         Log::Logger::info("\n");
                //     }
                // }
                // Log::Logger::info("\n");
                // Log::Logger::info("=============================\n");
            }
        }

        // for (const auto &block_pair : ffs_map) {
        //     const auto &fs_block = block_pair.second;

        //     // if (fs_block.data.get_size() > 30 && fs_block.data.get_size() < 100) {

        //     // } else {
        //     //     continue;
        //     // }

        //     print_fit_header(fs_block.header);
        //     Log::Logger::info("Data:\n");

        //     for (size_t i = 0; i < fs_block.data.get_size(); ++i) {
        //         Log::Logger::info("{:02X} ", (unsigned char) *(fs_block.data.get_data().get() + i));

        //         if ((i + 1) % 32 == 0) {
        //             Log::Logger::info("\n");
        //         }
        //     }
        //     Log::Logger::info("\n");
        //     Log::Logger::info("=============================\n");
        // }

        // const FFSBlock &    root_block      = ffs_map.at(0x14);
        // FileHeader          root_header     = read_file_header(root_block.data);

        // print_fit_header(root_block.header);
        // print_file_header(root_header);

        // const FFSBlock &    root_block      = ffs_map.at(3686);

        if (!ffs_map.count(10)) {
            throw Exception("Root block (ID: 10) not found. Broken filesystem?");
        }

        const FFSBlock &    root_block      = ffs_map.at(10);
        FileHeader          root_header     = read_file_header(root_block.data);
        Directory::Ptr      root            = Directory::build(root_header.name);

        scan(ffs_block_name, ffs_map, root, root_header);

        fs_map[ffs_block_name] = root;
    }
}

void NewSGOLD::read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id) {
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

    FFSBlock & block_data = ffs_map.at(data_id);

    data.add(block_data.data);

    if (part.next_part != 0xFFFFFFFF) {
        read_recurse(ffs_map, data, part.next_part);
    }
}

RawData NewSGOLD::read_full_data(FSBlocksMap &ffs_map, const FileHeader &header) {
    print_file_header(header);

    uint32_t data_id = header.id + 1;

    if (!ffs_map.count(data_id)) {
        throw Exception("Reading file data. Couldn't find block with id: {}", data_id);
    }

    const FFSBlock &block = ffs_map.at(data_id);

    RawData data_full(block.data);

    if (header.next_part != 0xFFFFFFFF) {
        read_recurse(ffs_map, data_full, header.next_part);
    }

    return data_full;
}

void NewSGOLD::scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, std::string path) {
    RawData data    = read_full_data(ffs_map, header);
    size_t  offset  = 0;

    while (offset < data.get_size()) {
        fflush(stdout);

        uint32_t raw;

        data.read<uint32_t>(offset, reinterpret_cast<char *>(&raw), 1);
        offset += 4;

        uint16_t    id  = raw & 0xFFFF;
        uint16_t    unk = (raw >> 16) & 0xFFFF;

        if (id == 0xFFFF) {
            break;
        }
        
        if (id == 0) {
            continue;
        }

        if (!ffs_map.count(id)) {
            throw Exception("FFS Block ID: {} not found", id);
        }

        const FFSBlock &    file_block      = ffs_map.at(id);
        FileHeader          file_header     = read_file_header(file_block.data);
        
        Log::Logger::info("Found ID: {:5d}, Path: {}{}", id, block_name, path + file_header.name);

        if (file_header.unknown6 & 0x10) {
            Directory::Ptr dir_next = Directory::build(file_header.name);

            dir->add_subdir(dir_next);

            scan(block_name, ffs_map, dir_next, file_header, path + file_header.name + "/");
        } else {
            try {
                RawData     file_data;
                uint32_t    data_id = file_header.id + 1;

                if (ffs_map.count(data_id)) {
                    file_data = read_full_data(ffs_map, file_header);
                }
            
                File::Ptr file = File::build(file_header.name, file_data);

                dir->add_file(file);
            } catch (const Exception &e) {
                Log::Logger::info("Warning! Broken file: {}", e.what());
            }
        }
    }
}

void NewSGOLD::unpack(Directory::Ptr dir, std::string path) {
    bool r = System::create_directory(path, 
                        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                        std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                        std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

    if (!r) {
        throw Exception("Couldn't create directory '{}'", path);
    }

    const auto &subdirs = dir->get_subdirs();
    const auto &files   = dir->get_files();

    for (const auto &file : files) {
        if (file->get_name().length() == 0) {
            continue;
        }

        std::string     file_path = path + "/" + file->get_name();
        std::ofstream   file_stream;

        Log::Logger::info("  Extracting {}", file_path);

        file_stream.open(file_path, std::ios_base::binary | std::ios_base::trunc);

        if (!file_stream.is_open()) {
            throw Exception("Couldn't create file '{}': {}", file_path, strerror(errno));
        }

        file_stream.write(file->get_data().get_data().get(), file->get_data().get_size());

        file_stream.close();
    }

    for (const auto &subdir : subdirs) {
        unpack(subdir, path + "/" + subdir->get_name());
    }

}

};
};
