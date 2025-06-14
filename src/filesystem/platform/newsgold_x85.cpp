#include "ffshit/filesystem/platform/newsgold_x85.h"

#include "ffshit/filesystem/ex.h"
#include "ffshit/filesystem/extract.h"
#include "ffshit/log/logger.h"

#include <iconv.h>
#include <sys/stat.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace FULLFLASH {
namespace Filesystem {

NewSGOLD_X85::NewSGOLD_X85(Blocks &blocks) : blocks(blocks) {
    parse_FIT();
}

void NewSGOLD_X85::load() {

}

static bool is_directory_exists(std::string path) {
    struct stat sb;

    if (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) { 
        return true;
    }

    return false;
}

void NewSGOLD_X85::extract(std::string path, bool overwrite) {
    FULLFLASH::Filesystem::extract(path, overwrite, [&](std::string dst_path) {
        for (const auto &fs : fs_map) {
            std::string fs_name = fs.first;
            auto root           = fs.second;

            Log::Logger::info("Extracting {}", fs_name);

            unpack(root, dst_path + "/" + fs_name);
        }
    });
}

void NewSGOLD_X85::print_fit_header(const NewSGOLD_X85::FITHeader &header) {
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

void NewSGOLD_X85::print_dir_header(const DirHeader &header) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("Dir header:");
    Log::Logger::debug("  ID:           {:04X} {}",     header.id, header.id);
    Log::Logger::debug("  Unknown1:     {:04X} {}",     header.unknown1, header.unknown1);
    Log::Logger::debug("  Unknown2:     {:04X} {}",     header.unknown2, header.unknown2);
    Log::Logger::debug("  Unknown3:     {:04X} {}",     header.unknown3, header.unknown3);
}

void NewSGOLD_X85::print_file_header(const NewSGOLD_X85::FileHeader &header) {
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

void NewSGOLD_X85::print_file_part(const FilePart &part) {
    Log::Logger::debug("===========================");
    Log::Logger::debug("Part:");
    Log::Logger::debug("  ID:             {:04X} {}", part.id, part.id);
    Log::Logger::debug("  Parent ID:      {:04X} {}", part.parent_id, part.parent_id);
    Log::Logger::debug("  Next part ID:   {:04X} {}", part.next_part, part.next_part);
}

RawData NewSGOLD_X85::read_aligned(const FFSBlock &block) {
    const auto &    data    = block.data;
    const auto &    fit     = block.header;

    char *          data_ptr    = data.get_data().get() + data.get_size();
    size_t          to_read     = fit.size;

    return read_aligned(data_ptr, fit.size);
}

RawData NewSGOLD_X85::read_aligned(char *data, size_t size) {
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

NewSGOLD_X85::FileHeader NewSGOLD_X85::read_file_header(const FFSBlock &block) {
    RawData                     header_data = read_aligned(block);
    NewSGOLD_X85::FileHeader    header;
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

    char *inptr = from;
    char *ouptr = to;

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

// NewSGOLD_X85::FileHeader NewSGOLD_X85::read_file_header2(const FFSBlock &block) {
//     RawData                     header_data = block.data_from_header;
//     NewSGOLD_X85::FileHeader    header;
//     size_t                      offset = 0;

//     header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.id), 1);
//     header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown1), 1);
//     // offset += 4;
//     header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
//     header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
//     header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown2), 1);
//     header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown4), 1);
//     header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown5), 1);
//     header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown6), 1);
//     header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown7), 1);

//     // ======================================================================

//     offset = 28;
//     size_t  str_size    = header_data.get_size() - 28;
//     size_t  from_n      = str_size;
//     char *  from        = new char[str_size];

//     size_t  to_n        = str_size;
//     char *  to          = new char[str_size];

//     header_data.read<char>(offset, from, from_n);

//     iconv_t iccd = iconv_open("UTF-8", "UTF-16LE");

//     if (iccd == ((iconv_t)-1)) {
//         throw Exception("iconv_open(): {}", strerror(errno));
//     }

//     char *inptr = from;
//     char *ouptr = to;

//     int r = iconv(iccd, &inptr, &from_n, &ouptr, &to_n);

//     if (r == -1) {
//         throw Exception("iconv(): {}", strerror(errno));
//     }

//     to[str_size - to_n] = 0x00;

//     header.name = std::string(to);

//     delete []from;
//     delete []to;

//     return header;
// }


NewSGOLD_X85::FilePart NewSGOLD_X85::read_file_part(const FFSBlock &block) {
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

void NewSGOLD_X85::dump_data(const RawData &raw_data) {
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

void NewSGOLD_X85::dump_block(const NewSGOLD_X85::FFSBlock &block, bool is_dump_data) {
    Log::Logger::debug("    ==================================================");
    Log::Logger::debug("    {} FFS Block addr: {:08X}, FIT Record addr {:08X}", block.block_ptr->name, block.ff_boffset, block.ff_offset);
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

void NewSGOLD_X85::parse_FIT() {
    Blocks::Map &bl = blocks.get_blocks();

    for (const auto &block : blocks.get_blocks()) {
        const auto &ffs_block_name  = block.first;
        const auto &ffs             = block.second;

        FSBlocksMap     ffs_map_C0;
        FSBlocksMapList ffs_map_00;

        if (ffs_block_name.find("FFS") != std::string::npos) {
            Log::Logger::debug("{} Blocks: {}", ffs_block_name, ffs.size());
        } else {
            continue;
        }

        for (auto &block : ffs) {
             Log::Logger::debug("  Block {:08X} {:08X} {:08X} {:08x} Size: {}",
                block.offset,
                block.header.unknown_1,
                block.header.unknown_2,
                block.header.unknown_3,
                block.data.get_size());

            std::vector<uint32_t> id_list_C0;
            std::vector<uint32_t> id_list_00;

            const RawData & block_data = block.data;
            size_t          block_size = block_data.get_size();
            size_t          fit_size = 0;

            size_t last_offset = block_size - 32;

            for (ssize_t offset = block_size - 64; offset > 0; offset -= 32) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk1), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk2), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk3), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.unk4), 1);

                // Конец fit табитцы. Корявое определение. Не очень понятно
                // как определять конец, или где находится её размер

                // uint32_t end_marker;
                
                // block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&end_marker), 1);

                if (fs_block.header.unk1 != 0xFFFFFFFF) {
                    fit_size = (block.offset + block_size) - (block.offset + offset) - 32;

                    break;
                }

                if (fs_block.header.flags == 0xFFFFFFC0 || fs_block.header.flags == 0xFFFFFF00) {
                    fs_block.data_header.add(block_data.get_data().get() + offset, last_offset - offset + 32);

                    // Log::Logger::debug("    {:08X}", block.offset + offset);

                    fs_block.ff_boffset = block.offset;
                    fs_block.ff_offset  = block.offset + offset;

                    last_offset = offset - 32;

                    size_t size_data    = ((fs_block.header.size / 16) + 1) * 32;
                    size_t offset_data  = offset - size_data;

                    fs_block.data = RawData(block_data, offset_data, size_data);
                    fs_block.block_ptr = &block;

                    if (fs_block.header.flags == 0xFFFFFFC0) {
                        if (ffs_map_C0.count(fs_block.header.id)) {
                            throw Exception("ffs_map_C0 Duplicate id {}", fs_block.header.id);
                        } else {
                            ffs_map_C0[fs_block.header.id] = fs_block;

                            id_list_C0.push_back(fs_block.header.id);
                        }
                    }

                    if (fs_block.header.flags == 0xFFFFFF00) {
                        if (fs_block.data_header.get_size() > 32) {
                            ssize_t data_size   = fs_block.header.size;
                            size_t  block_size  = fs_block.data_header.get_size();
                            size_t  offset = 32;

                            char *data_ptr = fs_block.data_header.get_data().get();

                            while (offset != block_size) {
                                char tmp[16];

                                memcpy(tmp, data_ptr + offset, 16);

                                fs_block.data_from_header.add(tmp, 16);

                                offset += 32;
                            }

                            // size_t offset        = fs_block.data_header.get_size() - 1;

                            // while (data_size != 0) {
                            //     offset -= 16;
                                
                            //     size_t to_read;

                            //     if (data_size - 16 > 0) {
                            //         to_read = 16;
                            //     } else {
                            //         to_read = data_size;
                            //     }

                            //     data_size -= to_read;
                            //     offset -= to_read - 1;

                            //     char tmp[16];

                            //     memcpy(tmp, fs_block.data_header.get_data().get() + offset, to_read);
                            //     // Log::Logger::debug("{}", to_read);
                            //     fs_block.data_from_header.add_top(tmp, to_read);
                            //     // fs_block.data_from_header.reverse(1);
                            // }

                            // fs_block.data_from_header = read_aligned(fs_block.data_header.get_data().get() + 32, fs_block.header.size);
                        }

                        ffs_map_00[fs_block.header.id].push_back(fs_block);

                        id_list_00.push_back(fs_block.header.id);
                    }

                    dump_block(fs_block);
                }
            }


            Log::Logger::debug("  C0 ID List:");
            for (const auto &id : id_list_C0) {
                Log::Logger::debug("    {}", id);
            }

            Log::Logger::debug("  00 ID List:");
            for (const auto &id : id_list_00) {
                Log::Logger::debug("    {}", id);
            }

        }

        if (!ffs_map_C0.count(10)) {
            throw Exception("Root block (ID: 10) not found. Broken filesystem?");
        }

        // return;
        Log::Logger::debug("ROOOOOOOT");
        const FFSBlock &    root_block      = ffs_map_C0.at(10);
        FileHeader          root_header     = read_file_header(root_block);

        dump_block(root_block);

        Directory::Ptr      root            = Directory::build(root_header.name);

        scan(ffs_map_C0, ffs_map_00, root, root_block);

        fs_map[ffs_block_name] = root;
    }
}

NewSGOLD_X85::DirList NewSGOLD_X85::get_directory(FSBlocksMap &ffs_map_C0, FSBlocksMapList &ffs_map_00, const FFSBlock &block) {
    DirList dir_list;

    if (!ffs_map_C0.count(block.header.id + 1)) {
        Log::Logger::warn("get_directory() block ID {} not found in ffs_map", block.header.id + 1);

        // exit(0);

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
            continue;;
        }
        
        if (dir_header.id == 0) {
            continue;
        }

        if (dir_header.unknown3 != 0xFFFF) {
            continue;
        }

        print_dir_header(dir_header);

        dir_list.push_back(dir_header);
        // fmt::print("ID: {:04X}, Unk: {:04X} Unk: {:04X} Unk: {:04X}\n", id, unk, unk2, unk3);
    }

    return dir_list;
}

uint32_t NewSGOLD_X85::read_part(const FFSBlock &prev_part, const FileHeader &file_header, const FSBlocksMap &ffs_map_C0, const FSBlocksMapList &ffs_map_00, uint32_t next_part, uint32_t last_offset, RawData &data, size_t start_offset) {
    if (!ffs_map_C0.count(next_part)) {
        throw Exception("read_part() block ID {} not found in ffs_map", next_part);
    }

    const FFSBlock &part_block  = ffs_map_C0.at(next_part);
    FilePart        part        = read_file_part(part_block);

    print_file_part(part);

    if (!ffs_map_C0.count(next_part + 1)) {
        throw Exception("read_part() block ID+1 {} not found in ffs_map", next_part + 1);
    }

    const FFSBlock &part_data = ffs_map_C0.at(next_part + 1);

    print_fit_header(part_data.header);

    // dump_block(part_data, false);

    // Log::Logger::debug("ID: {} Full size: {}, data size: {}, maybe size: {}", next_part, file_header.unknown2, data.get_size(), data.get_size() + part_data.header.size);
    // Log::Logger::debug("ID: {} Last offset: {:08X} part_data.header.offset: {:08X}", next_part, last_offset, part_data.header.offset);

    if (last_offset == part_data.header.offset) {
        Log::Logger::warn("Add end. Last offset: {:08X} part_data.header.offset: {:08X}\n", last_offset, part_data.header.offset);
        auto end = read_aligned(part_data);

        data.add(end);

        dump_data(end);

        return next_part;
    }

    // size_t data_addr = part_block.ff_boffset + part_data.header.offset + ((part_block.header.offset + part_block.fit_size) & 0x40000);
    // size_t data_addr = start_offset + part_data.header.offset;
    size_t data_addr = part_data.ff_boffset + part_data.header.offset;

    Log::Logger::debug("Data addr: {:08X} Block offset: {:08X} End data ddr: {:08X}", data_addr, part_data.ff_boffset, (data_addr + part_data.header.size) & 0xFFFFFF00);

    // if (part_block.header.offset != part_data.header.offset) {
    //     data_addr += part_block.header.offset + part_block.fit_size + 0x800;
    // }

    RawData file_data(this->blocks.get_data(), data_addr, part_data.header.size);
    data.add(file_data);

    // Log::Logger::debug("ID: {}, FF Block offset: {:08X} Part block offset {:08X} Size: {:08X} Part data offset {:08X} Size: {:08X} - Data addr: {:08X}", 
    //     next_part, part_block.ff_boffset, 
    //     part_block.header.offset, part_block.header.size, 
    //     part_data.header.offset, part_data.header.size, 
    //     data_addr);

    // dump_data(file_data);

    if (part.next_part != 0xFFFFFFFF) {
        return read_part(part_data, file_header, ffs_map_C0, ffs_map_00, part.next_part, last_offset, data, start_offset);
    }

    return next_part;
}

void NewSGOLD_X85::read_file(const FSBlocksMap &ffs_map_C0, const FSBlocksMapList &ffs_map_00, const FFSBlock &file_block, RawData &data) {
    const FITHeader &   fit_header  = file_block.header;
    FileHeader          file_header = read_file_header(file_block);;

    if (!ffs_map_C0.count(file_header.id + 1)) {
        throw Exception("Block {} not found", file_header.id + 1);
    }

    print_file_header(file_header);

    // if (file_header.name != "Bali_RiceTerraces .jpg") {
    //     return;
    // }

    dump_block(file_block);

    const FFSBlock &file_data_block = ffs_map_C0.at(file_header.id + 1);

    Log::Logger::debug("  {:08X} Start: {:04X}, {:08X}", file_data_block.ff_boffset, file_data_block.header.offset, file_data_block.ff_boffset + file_data_block.header.offset);
    // fmt::print("{:08X} {:04X} {:04X} Data offset: {:04X}\n", file_data.ff_boffset, file_data.header.offset, file_data.header.offset, file_data.ff_boffset + file_data.header.offset - file_data.header.offset);
    RawData file_data(this->blocks.get_data(), file_data_block.ff_boffset + file_data_block.header.offset, file_data_block.header.size);

    data.add(file_data);

    if (file_header.next_part != 0xFFFFFFFF) {
        uint32_t last_part = read_part(file_data_block, file_header, ffs_map_C0, ffs_map_00, file_header.next_part, fit_header.offset, data, file_data_block.ff_boffset);
    }

    if (file_header.next_part != 0xFFFFFFFF) {
        // const FFSBlock &    block2      = ffs_map.at(last_part + 1);
        // dump_block(block2);
        // RawData aligned = read_aligned(block2);
        // fmt::print("\n");
        // dump_data(aligned);
    }

}

void NewSGOLD_X85::scan(FSBlocksMap &ffs_map_C0, FSBlocksMapList &ffs_map_00, Directory::Ptr dir, const FFSBlock &block, std::string path) {
    auto dir_list = get_directory(ffs_map_C0, ffs_map_00, block);

    for (const auto &dir_info : dir_list) {
        const FFSBlock *    file_block;

        if (!ffs_map_C0.count(dir_info.id)) {
            Log::Logger::warn("scan() ID {} not found in ffs_map_C0", dir_info.id);
            continue;
            if (!ffs_map_00.count(dir_info.id)) {
                throw Exception("scan() ID {} not found in ffs_map_00", dir_info.id);
            }

            // file_block = &ffs_map_00.at(dir_info.id).at(0);

            // dump_block(*file_block);
            // Log::Logger::debug(" !!!!! ");
            // FileHeader          file_header = read_file_header2(*file_block);

            // print_file_header(file_header);
            // exit(0);

        } else {
            file_block  = &ffs_map_C0.at(dir_info.id);
        }

        const FITHeader &   fit_header  = file_block->header;

        // dump_block(*file_block);
        FileHeader          file_header = read_file_header(*file_block);;

        Log::Logger::debug("{:5d} {}", dir_info.id, path + file_header.name);
        // Log::Logger::debug("Block: {:08X}", file_block.ff_boffset);
        // Log::Logger::debug("FF Offset: {:08X}", file_block.ff_offset);

        // print_fit_header(file_block.header);
        // print_file_header(file_header);
        
        if (file_header.unknown6 & 0x10) {
            Directory::Ptr dir_next = Directory::build(file_header.name);

            dir->add_subdir(dir_next);

            scan(ffs_map_C0, ffs_map_00, dir_next, *file_block, path + file_header.name + "/");
        } else {
            try {
                RawData                 file_data;
                size_t                  size    = file_header.unknown2;

                if (file_header.unknown2 != 0) {
                    read_file(ffs_map_C0, ffs_map_00, *file_block, file_data);

                    if (size != file_data.get_size()) {
                        throw Exception("Result size {} != header size {}", file_data.get_size(), size);
                    }

                    File::Ptr file = File::build(file_header.name, file_data);

                    dir->add_file(file);
                }

            } catch (const Exception &e) {
                Log::Logger::warn("Warning! Broken file: {}\n", e.what());
            }
        }

    }
}

void NewSGOLD_X85::unpack(Directory::Ptr dir, std::string path) {
    int r = mkdir(path.c_str(), 0755);

    if (r == -1) {
        throw Exception("Couldn't create directory '{}': {}", path, strerror(errno));
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
