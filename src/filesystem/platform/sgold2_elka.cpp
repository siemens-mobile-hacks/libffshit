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

void SGOLD2_ELKA::load(bool skip_broken, bool skip_dup, bool dump_data) {
    parse_FIT(skip_broken, skip_dup, dump_data);
}

const Directory::Ptr SGOLD2_ELKA::get_root() const {
    return root_dir;
}

void SGOLD2_ELKA::print_fit_header(const SGOLD2_ELKA::FITHeader &header) {
    Log::Logger::debug("    ===========================");
    Log::Logger::debug("    FIT:");
    Log::Logger::debug("      Flags:      {:08X}",      header.flags);
    Log::Logger::debug("      ID:         {:08X} {:d}", header.id, header.id);
    Log::Logger::debug("      Size:       {:08X} {:d}", header.size, header.size);
    Log::Logger::debug("      Offset:     {:08X}",      header.offset);
    Log::Logger::debug("      Unk1:       {:08X} {:d}", header.unk1, header.unk1);
    Log::Logger::debug("      Unk2:       {:08X} {:d}", header.unk2, header.unk2);
    Log::Logger::debug("      Unk3:       {:08X} {:d}", header.unk3, header.unk3);
    Log::Logger::debug("      Unk4:       {:08X} {:d}", header.unk4, header.unk4);
}

void SGOLD2_ELKA::print_dir_header(const DirHeader &header) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("Dir header:");
    Log::Logger::debug("  ID:           {:04X} {}",     header.id, header.id);
    Log::Logger::debug("  Unknown1:     {:04X} {}",     header.unknown1, header.unknown1);
    Log::Logger::debug("  Unknown2:     {:04X} {}",     header.unknown2, header.unknown2);
    Log::Logger::debug("  Unknown3:     {:04X} {}",     header.unknown3, header.unknown3);
}

void SGOLD2_ELKA::print_file_header(const SGOLD2_ELKA::FileHeader &header) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("File:");
    Log::Logger::debug("  ID:           {:04X} {}",     header.id, header.id);
    Log::Logger::debug("  Unknown1:     {:04X} {}",     header.unknown1, header.unknown1);
    Log::Logger::debug("  Parent ID:    {:04X} {}",     header.parent_id, header.parent_id);
    Log::Logger::debug("  Next part ID: {:04X} {}",     header.next_part, header.next_part);
    Log::Logger::debug("  Size:         {:08X} {}",     header.unknown2, header.unknown2);
    Log::Logger::debug("  Unknown4:     {:04X} {}",     header.unknown4, header.unknown4);
    Log::Logger::debug("  Unknown5:     {:04X} {}",     header.unknown5, header.unknown5);
    Log::Logger::debug("  Unknown6:     {:04X} {}",     header.unknown6, header.unknown6);
    Log::Logger::debug("  Unknown7:     {:04X} {}",     header.unknown7, header.unknown7);
    Log::Logger::debug("  Name:         {}",            header.name);
}

void SGOLD2_ELKA::print_file_header2(const SGOLD2_ELKA::FileHeader &header) {
    Log::Logger::warn("===========================");
    Log::Logger::warn("File:");
    Log::Logger::warn("  ID:           {:04X} {}",     header.id, header.id);
    Log::Logger::warn("  Unknown1:     {:04X} {}",     header.unknown1, header.unknown1);
    Log::Logger::warn("  Parent ID:    {:04X} {}",     header.parent_id, header.parent_id);
    Log::Logger::warn("  Next part ID: {:04X} {}",     header.next_part, header.next_part);
    Log::Logger::warn("  Size:         {:08X} {}",     header.unknown2, header.unknown2);
    Log::Logger::warn("  Unknown4:     {:04X} {}",     header.unknown4, header.unknown4);
    Log::Logger::warn("  Unknown5:     {:04X} {}",     header.unknown5, header.unknown5);
    Log::Logger::warn("  Unknown6:     {:04X} {}",     header.unknown6, header.unknown6);
    Log::Logger::warn("  Unknown7:     {:04X} {}",     header.unknown7, header.unknown7);
    Log::Logger::warn("  Name:         {}",            header.name);
}

void SGOLD2_ELKA::print_file_part(const FilePart &part) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("Part:");
    Log::Logger::debug("  ID:             {:04X} {}", part.id, part.id);
    Log::Logger::debug("  Parent ID:      {:04X} {}", part.parent_id, part.parent_id);
    Log::Logger::debug("  Next part ID:   {:04X} {}", part.next_part, part.next_part);
}

RawData SGOLD2_ELKA::read_aligned(const FFSBlock &block) {
    const auto &    data    = block.data;
    const auto &    fit     = block.header;

    char *          data_ptr    = data.get_data().get() + data.get_size();
    size_t          to_read     = fit.size;

    return read_aligned(data_ptr, fit.size);
}

RawData SGOLD2_ELKA::read_aligned(const FFSBlock &block, size_t size) {
    const auto &    data    = block.data;
    const auto &    fit     = block.header;

    char *          data_ptr    = data.get_data().get() + data.get_size();

    return read_aligned(data_ptr, size);
}

RawData SGOLD2_ELKA::read_aligned(char *data, size_t size) {
    RawData         data_ret;

    char *          data_ptr    = data;
    size_t          to_read     = size;

    while(to_read != 0) {
        size_t read_size    = 16;
        size_t skip         = 0;

        if (to_read > 16) {
            // read_size = 16;
        } else {
            // read_size   = to_read;
            skip        = 16 - to_read;
        }

        data_ptr    -= 32;
        data_ret.add_top(data_ptr + skip, read_size - skip);
        to_read     -= read_size - skip;
    }

    return data_ret;
}

SGOLD2_ELKA::FileHeader SGOLD2_ELKA::read_file_header(const FFSBlock &block) {
    RawData                     header_data = read_aligned(block);
    SGOLD2_ELKA::FileHeader    header;
    size_t                      offset = 0;

    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown1), 1);
    // offset += 4;
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown2), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown4), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown5), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown6), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown7), 1);

    // ======================================================================

    offset = 28;
    size_t  str_size    = header_data.get_size() - 28;
    size_t  from_n      = str_size;
    char *  from        = new char[str_size];

    size_t  to_n        = str_size;
    char *  to          = new char[str_size];

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
        throw Exception("iconv(): {}", strerror(errno));
    }

    to[str_size - to_n] = 0x00;

    header.name = std::string(to);

    delete []from;
    delete []to;

    return header;
}

SGOLD2_ELKA::FilePart SGOLD2_ELKA::read_file_part(const FFSBlock &block) {
   RawData         header_data;

    const auto &    data    = block.data;
    const auto &    fit     = block.header;
    
    char *          data_ptr    = data.get_data().get() + data.get_size();
    size_t          to_read     = fit.size;

    while(to_read != 0) {
        size_t read_size    = 16;
        size_t skip         = 0;

        if (to_read > 16) {
            // read_size = 16;
        } else {
            // read_size   = to_read;
            skip        = 16 - to_read;
        }

        data_ptr    -= 32;
        header_data.add_top(data_ptr + skip, read_size - skip);
        to_read     -= read_size - skip;
    }

    // ======================================================================

    FilePart    part;
    size_t      offset = 0;

    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.parent_id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.next_part), 1);

    return part;
}

void SGOLD2_ELKA::dump_data(const RawData &raw_data) {
    if (!raw_data.get_data()) {
        throw Exception("raw_data == nullptr");
    }

    bool is_aligned = false;

    std::string bin_print;

    for (size_t i = 0; i < raw_data.get_size(); ++i) {
        is_aligned = false;

        bin_print += fmt::format("{:02X} ", (unsigned char) *(raw_data.get_data().get() + i));

        if ((i + 1) % 16 == 0) {
            Log::Logger::debug("{}", bin_print);
            bin_print.clear();

            is_aligned = true;
        }
    }

    if (!is_aligned) {
        Log::Logger::debug("{}", bin_print);
    }
}

void SGOLD2_ELKA::dump_block(const SGOLD2_ELKA::FFSBlock &block, bool is_dump_data) {
    Log::Logger::debug("    ==================================================");
    Log::Logger::debug("    {} FFS Block addr: {:08X}, FIT Record addr {:08X}", block.block_ptr->get_header().name, block.ff_boffset, block.ff_offset);
    print_fit_header(block.header);

    Log::Logger::debug("Header data (size: {}): ", block.data_header.get_size());

    if (is_dump_data) {
        dump_data(block.data_header);

        if (block.data_from_header.get_size() > 0) {
            Log::Logger::debug("Header aligned data (size: {}): ", block.data_from_header.get_size());

            dump_data(block.data_from_header);
        }
    }

    Log::Logger::debug("Data (size: {}):", block.data.get_size());

    if (is_dump_data) {
        dump_data(block.data);
    }
}

void SGOLD2_ELKA::parse_FIT(bool skip_broken, bool skip_dup, bool dump_data) {
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

        FSBlocksMap     ffs_map_C0;

        for (const auto &block : part_blocks) {
            const auto &block_header = block.get_header();

             Log::Logger::debug("  Block {:08X} {:08X} {:08X} {:08x} Size: {}",
                block.get_addr(),
                block_header.unknown_1,
                block_header.unknown_2,
                block_header.unknown_3,
                block.get_size());

            const RawData & block_data = block.get_data();
            size_t          block_size = block.get_size();
            size_t          fit_size = 0;

            size_t last_offset = block_size - 32;

            for (ssize_t offset = block_size - 64; offset > 0; offset -= 16) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                // block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk1), 1);
                // block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk2), 1);
                // block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk3), 1);
                // block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk4), 1);

                if (fs_block.header.flags == 0xFFFFFFFF) {
                    continue;
                }

                if (fs_block.header.flags != 0xFFFFFFC0) {
                    continue;
                }

                fs_block.data_header.add(block_data.get_data().get() + offset, last_offset - offset + 32);

                // Log::Logger::debug("    {:08X}", block.offset + offset);

                fs_block.ff_boffset = block.get_addr();
                fs_block.ff_bsize   = block.get_size();
                fs_block.ff_offset  = block.get_addr() + offset;

                last_offset = offset - 32;

                // if (fs_block.header.size != 0x800) {
                    size_t size_data    = ((fs_block.header.size / 16) + 1) * 32;
                    size_t offset_data  = offset - size_data;

                    fs_block.data       = RawData(block_data, offset_data, size_data);
                // }

                fs_block.block_ptr  = &block;

                if (ffs_map_C0.count(fs_block.header.id)) {
                    throw Exception("ffs_map_C0 Duplicate id {}", fs_block.header.id);
                }

                ffs_map_C0[fs_block.header.id] = fs_block;
                
                dump_block(fs_block, dump_data);
            }
        }

        if (!ffs_map_C0.count(10)) {
            throw Exception("Root block (ID: 10) not found. Broken filesystem?");
        }

        // return;
        Log::Logger::debug("ROOOOOOOT");
        const FFSBlock &    root_block      = ffs_map_C0.at(10);
        FileHeader          root_header     = read_file_header(root_block);

        dump_block(root_block, dump_data);

        Directory::Ptr      root            = Directory::build(part_name, ROOT_PATH);

        root_dir->add_subdir(root);

        scan(part_name, ffs_map_C0, root, root_block);
    }
}

SGOLD2_ELKA::DirList SGOLD2_ELKA::get_directory(FSBlocksMap &ffs_map_C0, const FFSBlock &block) {
    DirList dir_list;

    if (!ffs_map_C0.count(block.header.id + 1)) {
        Log::Logger::warn("get_directory() block ID {} not found in ffs_map", block.header.id + 1);

        return dir_list;
    }

    const FFSBlock &dir_block  = ffs_map_C0.at(block.header.id + 1);

    dump_block(dir_block);

    const auto &    dir_data = dir_block.data;
    const size_t    dir_data_size = dir_block.data.get_size();

    RawData data;

    size_t offset = 0;

    while (offset < dir_data_size) {
        data.add(dir_data.get_data().get() + offset, 16);
        offset += 32;
    }

    // =======================================================

    offset = 0;

    while (offset < data.get_size()) {
        DirHeader dir_header;

        data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.id), 1);
        data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.unknown1), 1);
        data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.unknown2), 1);
        data.read<uint16_t>(offset, reinterpret_cast<char *>(&dir_header.unknown3), 1);

        if (dir_header.id == 0xFFFF) {
            continue;
        }
        
        if (dir_header.id == 0) {
            continue;
        }

        if (dir_header.unknown3 != 0xFFFF) {
            continue;
        }

        print_dir_header(dir_header);

        dir_list.push_back(dir_header);
    }

    return dir_list;
}

uint32_t SGOLD2_ELKA::read_part(const FFSBlock &prev_part, const FileHeader &file_header, const FSBlocksMap &ffs_map_C0, uint32_t next_part, uint32_t last_offset, RawData &data, size_t start_offset) {
    if (!ffs_map_C0.count(next_part)) {
        throw Exception("read_part() block ID {} not found in ffs_map", next_part);
    }

    const FFSBlock &part_block  = ffs_map_C0.at(next_part);
    FilePart        part        = read_file_part(part_block);

    print_file_part(part);

    if (!ffs_map_C0.count(part.id + 1)) {
        throw Exception("read_part() block ID+1 {} not found in ffs_map", part.id + 1);
    }

    const FFSBlock &part_data = ffs_map_C0.at(part.id + 1);

    print_fit_header(part_data.header);
    Log::Logger::debug("Part data size: {}", part_data.data.get_size());

    if (last_offset == part_data.header.offset) {
        Log::Logger::warn("{} Add end. Last offset: {:08X} part_data.header.offset: {:08X}", file_header.name, last_offset, part_data.header.offset);

        auto end = read_aligned(part_data);

        data.add(end);
    } else {
        size_t size_diff = part_data.header.size - 0x400;

        if ( size_diff < 0x200 ) {
            size_t data_addr = part_data.ff_boffset + part_data.header.offset;

            Log::Logger::debug("Data addr: {:08X} Block offset: {:08X}, Block end: {:08X} End data ddr: {:08X}", data_addr, part_data.ff_boffset, part_data.ff_boffset + part_data.ff_bsize, (data_addr + 0x400));

            RawData file_data(this->partitions->get_data(), data_addr, 0x400);

            data.add(file_data);

            auto end = read_aligned(part_data, part_data.header.size - 0x400);

            data.add(end);

        } else {
            size_t data_addr = part_data.ff_boffset + part_data.header.offset;
            Log::Logger::debug("Data addr: {:08X} Block offset: {:08X}, Block end: {:08X} End data ddr: {:08X}", data_addr, part_data.ff_boffset, part_data.ff_boffset + part_data.ff_bsize, (data_addr + part_data.header.size));

            RawData file_data(this->partitions->get_data(), data_addr, part_data.header.size);

            data.add(file_data);
        }
    }

    if (part.next_part != 0xFFFFFFFF) {
        return read_part(part_data, file_header, ffs_map_C0, part.next_part, last_offset, data, start_offset);
    }

    return next_part;
}

void SGOLD2_ELKA::read_file(const FSBlocksMap &ffs_map_C0, const FFSBlock &file_block, RawData &data) {
    const FITHeader &   fit_header  = file_block.header;
    FileHeader          file_header = read_file_header(file_block);;

    if (!ffs_map_C0.count(file_header.id + 1)) {
        throw Exception("Block {} not found", file_header.id + 1);
    }

    print_file_header(file_header);

    dump_block(file_block);

    const FFSBlock &file_data_block = ffs_map_C0.at(file_header.id + 1);

    Log::Logger::debug("  {:08X} Start: {:04X}, {:08X}", file_data_block.ff_boffset, file_data_block.header.offset, file_data_block.ff_boffset + file_data_block.header.offset);
    Log::Logger::debug(" {:04X}", file_data_block.header.size - 0x400);

    size_t size_diff = file_data_block.header.size - 0x400;

    if (file_header.next_part == 0xFFFFFFFF && size_diff < 0x200) {
        RawData file_data(this->partitions->get_data(), file_data_block.ff_boffset + file_data_block.header.offset, 0x400);

        data.add(file_data);

        // sddd
        auto end = read_aligned(file_data_block, file_data_block.header.size - 0x400);

        data.add(end);

    } else {
        RawData file_data(this->partitions->get_data(), file_data_block.ff_boffset + file_data_block.header.offset, file_data_block.header.size);

        data.add(file_data);
    }


    if (file_header.next_part != 0xFFFFFFFF) {
        read_part(file_data_block, file_header, ffs_map_C0, file_header.next_part, fit_header.offset, data, file_data_block.ff_boffset);
    }
}

void SGOLD2_ELKA::scan(const std::string &block_name, FSBlocksMap &ffs_map_C0, Directory::Ptr dir, const FFSBlock &block, std::string path) {
    auto dir_list = get_directory(ffs_map_C0, block);

    for (const auto &dir_info : dir_list) {
        const FFSBlock *    file_block;

        if (!ffs_map_C0.count(dir_info.id)) {
            throw Exception("scan() ID {} not found in ffs_map_C0", dir_info.id);
        }

        file_block  = &ffs_map_C0.at(dir_info.id);

        const FITHeader &   fit_header  = file_block->header;

        // dump_block(*file_block);
        FileHeader          file_header = read_file_header(*file_block);;

        Log::Logger::debug("{:5d} {}", dir_info.id, path + file_header.name);
        
        if (file_header.unknown6 & 0x10) {
            Directory::Ptr dir_next = Directory::build(file_header.name, block_name + path);

            dir->add_subdir(dir_next);

            scan(block_name, ffs_map_C0, dir_next, *file_block, path + file_header.name + "/");
        } else {
            try {
                RawData                 file_data;
                size_t                  size    = file_header.unknown2;

                if (file_header.unknown2 != 0) {
                    read_file(ffs_map_C0, *file_block, file_data);

                    if (size != file_data.get_size()) {
                        throw Exception("Result size {} != header size {}", file_data.get_size(), size);
                    }

                    File::Ptr file = File::build(file_header.name, block_name + path, file_data);

                    dir->add_file(file);
                }

            } catch (const Exception &e) {
                print_file_header2(file_header);
                Log::Logger::warn("Warning! Broken file {:04X} '{}': {}", file_header.unknown6, path + file_header.name, e.what());
            }
        }

    }
}

};
};
