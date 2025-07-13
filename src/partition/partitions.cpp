/* =================================================================
   |                                                               |
   |  Thanks to Azq2, marry_on_me for partitions search algorithm  |
   |                                                               |
   |                   ♡♡♡ Love you guys ♡♡♡                      |
   |                                                               |
   ================================================================= */

#include "ffshit/help.h"
#include "ffshit/system.h"

#include "ffshit/partition/partitions.h"
#include "ffshit/partition/ex.h"
#include "ffshit/log/logger.h"

#include <sstream>
#include <algorithm>

namespace FULLFLASH {
namespace Partitions {

static const Patterns::Readable pattern_sg {
    "?? ?? ?? A?", // name ptr
    "?? ?? 00 00",
    "?? ?? 00 00",
    "?? ?? ?? A?",
    "?? ?? 00 00",
    "?? ?? 00 00", // table size
    "?? ?? ?? A?", // table ptr
    "?? ?? ?? ??",
    "?? ?? ?? A?",
    "?? ?? ?? A?", // unk func
    "?? ?? ?? ??",
};

static const Patterns::Readable pattern_nsg {
    "?? ?? ?? A?", // name ptr
    "?? ?? 00 00",
    "?? ?? 00 00",
    "?? ?? ?? ??",
    "?? ?? ?? ??",
    "?? ?? ?? ??",
    "?? ?? ?? A?",
    "?? ?? 00 00",
    "?? ?? 00 00", // table size
    "?? ?? ?? A?", // table
    "?? ?? ?? ??",
    "?? ?? ?? A?",
    "?? ?? ?? ??",
};

static const Patterns::Readable pattern_egold {
    // "FF FF FF FF",
    // "FF FF FF FF",
    // "FF FF FF FF",
    // "FF FF FF FF",
    "FE FE ?? ??",
    "?? ?? ?? ??",
    "?? ?? ?? ??",
    "?? ?? FE FE",
};

static const std::vector<std::string> possible_part_names {
    "BCORE",
    "EEFULL",
    "EELITE",
    "EXIT",
    "FFS",
    "UNUSED",
    "__FM__",
    "RIM",
};

static size_t search_end(const char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        unsigned char data = buf[i];

        if (data == 0x0) {
            return i;
        }
    }

    return size;
}

static bool is_printable(const char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (isprint(static_cast<unsigned char>(buf[i]))) {
            continue;;
        }

        return false;
    }

    return true;
}

static bool is_empty(const char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (static_cast<const uint8_t>(buf[i]) != 0xFF) {
            return false;
        }
    }

    return true;
}

// =========================================================================

Partitions::Partitions(std::filesystem::path fullflash_path, bool old_search_algorithm, uint32_t search_start_addr) {
    sl75_bober_kurwa = false;
    
    this->fullflash_path = fullflash_path;

    std::ifstream file;

    file.open(fullflash_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw Exception("Couldn't open file '{}': {}\n", fullflash_path.string(), std::string(strerror(errno)));
    }

    file.seekg(0, std::ios_base::end);
    size_t data_size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    this->data = RawData(file, 0, data_size);

    detect_platform();
    search_partitions(old_search_algorithm, search_start_addr);

    Log::Logger::debug("Found {} partitions", partitions_map.size());

    for (const auto &pair : partitions_map) {
        Log::Logger::debug("  {:8s} {}", pair.first, pair.second.get_blocks().size());
    }

}

Partitions::Partitions(std::filesystem::path fullflash_path, Platform platform, bool old_search_algorithm, uint32_t search_start_addr) {
    std::ifstream file;

    this->fullflash_path = fullflash_path;

    file.open(fullflash_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw Exception("Couldn't open file '{}': {}\n", fullflash_path.string(), std::string(strerror(errno)));
    }

    file.seekg(0, std::ios_base::end);
    size_t data_size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    this->data      = RawData(file, 0, data_size);
    this->platform  = platform;
    this->model     = PlatformToString.at(platform);

    search_partitions(old_search_algorithm, search_start_addr);

    for (const auto &pair : partitions_map) {
        Log::Logger::debug("{:10s} {}", pair.first, pair.second.get_blocks().size());
    }
}


Partitions::Partitions(char *ff_data, size_t ff_data_size, bool old_search_algorithm, uint32_t search_start_addr) {
    sl75_bober_kurwa = false;
    
    this->data = RawData(ff_data, ff_data_size);

    detect_platform();
    search_partitions(old_search_algorithm, search_start_addr);

    Log::Logger::debug("Found {} partitions", partitions_map.size());

    for (const auto &pair : partitions_map) {
        Log::Logger::debug("  {:8s} {}", pair.first, pair.second.get_blocks().size());
    }
}

Partitions::Partitions(char *ff_data, size_t ff_data_size, Platform platform, bool old_search_algorithm, uint32_t search_start_addr) {
    std::ifstream file;

    this->data      = RawData(ff_data, ff_data_size);
    this->platform  = platform;
    this->model     = PlatformToString.at(platform);

    search_partitions(old_search_algorithm, search_start_addr);

    for (const auto &pair : partitions_map) {
        Log::Logger::debug("{:10s} {}", pair.first, pair.second.get_blocks().size());
    }
}

const std::filesystem::path &Partitions::get_file_path() const {
    return fullflash_path;
}

const Partitions::Map &Partitions::get_partitions() const {
    return partitions_map;
}

const RawData & Partitions::get_data() const {
    return data;
}

const Platform Partitions::get_platform() const {
    return platform;
}

const std::string &Partitions::get_imei() const {
    return imei;
}

const std::string &Partitions::get_model() const {
    return model;
}

void Partitions::search_partitions(bool old_search_algorithm, uint32_t start_addr) {
    auto old_search = [&]() {
        switch (platform) {
            case Platform::EGOLD_CE:    block_size = 0x10000; old_search_partitions_egold_ce(); break;
            case Platform::SGOLD:
            case Platform::SGOLD2:      block_size = 0x10000; old_search_partitions_sgold_sgold2(); break;
            case Platform::SGOLD2_ELKA: block_size = 0x10000; old_search_partitions_sgold2_elka(); break;
            default: throw Exception("Couldn't detect fullflash platform");
        }
    };

    auto new_search = [&]() {
        switch (platform) {
            case Platform::EGOLD_CE: {
                Log::Logger::warn("New partitions search algorithm not implemented yet for EGOLD_CE");

                old_search();

                break;
            }
            case Platform::SGOLD:   search_partitions_sgold(start_addr); break;
            case Platform::SGOLD2:  search_partitions_sgold2(start_addr); break;
            case Platform::SGOLD2_ELKA:  {
                Log::Logger::warn("New partitions search algorithm not implemented yet for SGOLD2_ELKA");

                block_size = 0x10000;

                old_search_partitions_sgold2_elka();

                break;
            }
            default: throw Exception("Couldn't detect fullflash platform");
        }
    };

    if (old_search_algorithm) {
        old_search();

        return;
    }

    new_search();

    if (!partitions_map.size()) {
        Log::Logger::warn("Partitions not found. Trying to use old search algorithm");

        old_search();
    } else {
        bool ffs_partitions_found = false;

        for (const auto &name : partitions_map) {
            if (name.first.find("FFS") != std::string::npos) {
                ffs_partitions_found = true;
            }
        }

        if (!ffs_partitions_found) {
            Log::Logger::warn("FFS Partitions not found. Trying to use old search algorithm");

            old_search();
        }
    }

    if (!partitions_map.size()) {
        throw Exception("Partitions not found");
    }
}

bool Partitions::check_part_name(const std::string &name) {
    if (name.size() > 8) {
        return false;
    }

    for (const auto &pname : possible_part_names) {
        if (name.find(pname) != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool Partitions::search_partitions_sgold(uint32_t start_addr) {
    Log::Logger::debug("Searching partitions from 0x{:08X}", start_addr);
    Log::Logger::debug("Searching pattern");

    auto addresses = find_pattern(pattern_sg, start_addr, false);

    Log::Logger::debug("Pattern searching end");

    if (!addresses.size()) {
        return false;
    }

    for (auto &addr : addresses) {
        Log::Logger::debug("Pattern find, addr: {:08X}", addr);

        size_t struct_size = 0x2C;

        for (size_t offset = addr; offset < addr + 64 * struct_size; offset += struct_size) {
            RawData raw_data(data.get_data().get() + offset, struct_size);

            uint32_t name_addr;
            uint32_t table_size;
            uint32_t table_addr;

            data.read_type<uint32_t>(offset + 0x00, &name_addr);
            data.read_type<uint32_t>(offset + 0x14, &table_size);
            data.read_type<uint32_t>(offset + 0x18, &table_addr);

            if ((name_addr & 0xF0000000) != 0xA0000000) {
                break;
            }
            if ((table_addr & 0xF0000000) != 0xA0000000) {
                break;
            }

            if (!table_size) {
                continue;
            }

            name_addr   &= FF_ADDRESS_MASK;
            table_addr  &= FF_ADDRESS_MASK;

            std::string partition_name;

            data.read_string(name_addr, partition_name);

            if (!is_printable(partition_name.data(), partition_name.size())) {
                continue;
            }

            if (partition_name.find(" ") != std::string::npos) {
                continue;
            }

            if (!check_part_name(partition_name)) {
                continue;
            }

            Log::Logger::debug("Header. Name addr: {:08X}, Table addr: {:08X}, size {:08X}, {}", name_addr, table_addr, table_size, partition_name);

            for (size_t i = 0; i < table_size * 8; i += 8) {
                uint32_t block_addr;
                uint32_t block_size;

                data.read_type<uint32_t>(table_addr + i, &block_addr);
                data.read_type<uint32_t>(table_addr + i + 4, &block_size);

                uint32_t masked_block_addr = block_addr & FF_ADDRESS_MASK;
                uint32_t masked_block_size = block_size & FF_ADDRESS_MASK;

                Log::Logger::debug("  Block:        Name: {}, Addr: {:08X}, size: {:08X}, table: {:08X}", partition_name, masked_block_addr, masked_block_size, table_addr + i);

                if (!partitions_map.count(partition_name)) {
                    partitions_map[partition_name] = Partition(partition_name);
                }

                if (partition_name.find("FFS") != std::string::npos) {
                    Block::Header header;

                    size_t  block_rd_offset = masked_block_addr;

                    data.read<char>(block_rd_offset, header.name, 8);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
                    data.read<uint32_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_3), 1);
                    header.unknown_4 = 0x0;

                    if (header.unknown_3 != 0xFFFFFFF0) {
                        memset(header.name, 0x0, 8);

                        Log::Logger::warn("Skip. The patch for increasing the disk size has been installed, but the blocks have not been formatted? ");

                        continue;
                    }

                    Log::Logger::debug("  Block header: Name: {}, Unk1: {:04X}, Unk2: {:04X}, Unk3: {:08X}", header.name, header.unknown_1, header.unknown_2, header.unknown_3);

                    partitions_map[partition_name].add_block(
                        Block(  header, 
                                RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                                masked_block_addr, 
                                masked_block_size)
                    );
                } else {
                    Block::Header header;

                    // ну пусть будет
                    size_t  block_rd_offset = masked_block_addr;

                    data.read<char>(block_rd_offset, header.name, 8);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
                    data.read<uint32_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

                    partitions_map[partition_name].add_block(
                        Block(  header, 
                                RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                                masked_block_addr, 
                                masked_block_size)
                    );

                }
            }
        }

        if (!partitions_map.empty()) {
            break;
        }
    }

    return true;
}

bool Partitions::search_partitions_sgold2(uint32_t start_addr) {
    Log::Logger::debug("Searching partitions from 0x{:08X}", start_addr);

    // auto address_list    = find_pattern(pattern_nsg);
    auto address_list    = find_pattern(pattern_nsg, start_addr);
    
    if (sl75_bober_kurwa) {
        Log::Logger::warn("SL75 ja pierdole!");
    }

    if (!address_list.size()) {
        return false;
    }

    for (auto &addr : address_list) {
        Log::Logger::debug("Pattern find, addr: {:08X}", addr);

        size_t struct_size = 0x34;

        for (size_t offset = addr; offset < addr + 64 * struct_size; offset += struct_size) {
            uint32_t name_addr;
            uint32_t table_size;
            uint32_t table_addr;

            if (offset + struct_size >= data.get_size()) {
                break;
            }

            data.read_type<uint32_t>(offset + 0x00, &name_addr);
            data.read_type<uint32_t>(offset + 0x20, &table_size);
            data.read_type<uint32_t>(offset + 0x24, &table_addr);

            if ((name_addr  & 0xF0000000) != 0xA0000000) {
                break;
            }
            if ((table_addr & 0xF0000000) != 0xA0000000) {
                break;
            }

            if (!table_size) {
                continue;
            }

            name_addr   &= FF_ADDRESS_MASK;
            table_addr  &= FF_ADDRESS_MASK;

            std::string partition_name;

            data.read_string(name_addr, partition_name);

            if (!is_printable(partition_name.data(), partition_name.size())) {
                continue;
            }

            if (partition_name.find(" ") != std::string::npos) {
                continue;
            }

            if (!check_part_name(partition_name)) {
                continue;
            }

            Log::Logger::debug("Header. Name addr: {:08X}, Table addr: {:08X}, size {:08X}, {}", name_addr, table_addr, table_size, partition_name);

            for (size_t i = 0; i < table_size * 8; i += 8) {
                uint32_t block_addr;
                uint32_t block_size;

                if (table_addr + i >= data.get_size()) {
                    break;
                }

                if (table_addr + i + 4>= data.get_size()) {
                    break;
                }

                data.read_type<uint32_t>(table_addr + i, &block_addr);
                data.read_type<uint32_t>(table_addr + i + 4, &block_size);

                if (((block_addr & 0xFF000000) > 0xA2000000) && sl75_bober_kurwa) {
                    if (!sl75_bober_kurwa) {
                        // throw Exception("!SL75_ja_pierdole?");
                        break;
                    }

                    uint32_t shit = 0xA4000000 - 0xA2000000;
                    Log::Logger::debug("  JA PIERDOLE! {:08X} -> {:08X}", block_addr, block_addr - shit);

                    block_addr -= shit;
                }

                uint32_t masked_block_addr = block_addr & FF_ADDRESS_MASK;
                uint32_t masked_block_size = block_size & FF_ADDRESS_MASK;

                Log::Logger::debug("  Block:        Name: {}, Addr: {:08X}, size: {:08X}, table: {:08X}", partition_name, masked_block_addr, masked_block_size, table_addr + i);

                if (!partitions_map.count(partition_name)) {
                    partitions_map[partition_name] = Partition(partition_name);
                }

                if (partition_name.find("FFS") != std::string::npos) {
                    Block::Header header;

                    size_t  block_rd_offset = masked_block_addr;

                    data.read<char>(block_rd_offset, header.name, 8);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
                    data.read<uint32_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

                    if (header.unknown_3 != 0xFFFFFFF0) {
                        memset(header.name, 0x0, 8);

                        Log::Logger::warn("Skip. The patch for increasing the disk size has been installed, but the blocks have not been formatted? ");

                        continue;
                    }

                    Log::Logger::debug("  Block header: Name: {}, Unk1: {:04X}, Unk2: {:04X}, Unk3: {:08X}", header.name, header.unknown_1, header.unknown_2, header.unknown_3);

                    partitions_map[partition_name].add_block(
                        Block(  header, 
                                RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                                masked_block_addr, 
                                masked_block_size)
                    );
                } else {
                    Block::Header header;

                    // ну пусть будет
                    size_t  block_rd_offset = masked_block_addr;

                    data.read<char>(block_rd_offset, header.name, 8);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
                    data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
                    data.read<uint32_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

                    partitions_map[partition_name].add_block(
                        Block(  header, 
                                RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                                masked_block_addr, 
                                masked_block_size)
                    );
                }
            }
        }

        if (!partitions_map.empty()) {
            break;
        }
    }

    return true;
}

bool Partitions::search_partitions_sgold2_elka(uint32_t start_addr) {
    return false;
}

void Partitions::detect_platform() {
    std::string bc;

    data.read_string(BC65_BC75_OFFSET, bc, 1);

    if (bc == "BC65" || bc == "BCORE65") {
        platform = Platform::SGOLD;

        data.read_string(X65_MODEL_OFFSET, model);
        data.read_string(X65_IMEI_OFFSET, imei);

        if (imei.length() != 15) {
            imei.clear();
            data.read_string(X65_7X_IMEI_OFFSET, imei);
        }
    } else if (bc == "BC75") {
        platform = Platform::SGOLD2;

        data.read_string(X75_MODEL_OFFSET, model);
        data.read_string(X75_IMEI_OFFSET, imei);
    } else {
        bc.clear();

        data.read_string(BC85_OFFSET, bc, 1);

        if (bc == "BC85") {
            platform = Platform::SGOLD2_ELKA;

            data.read_string(X85_MODEL_OFFSET, model);
            data.read_string(X85_IMEI_OFFSET, imei);

        } else {
            platform = Platform::UNK;
        }
    }


    if (platform == Platform::UNK) {
        for (const auto &offset : EGOLD_INFO_OFFSETS) {
            std::string magick;

            data.read_string(offset + EGOLD_MAGICK_SIEMENS_OFFSET, magick);

            if (magick != "SIEMENS") {
                continue;
            }

            data.read_string(offset + EGOLD_MODEL_OFFSET, model);

            platform = Platform::EGOLD_CE;

            break;
        }
    }

    if (platform == Platform::SGOLD2) {
        std::string model_local(model);

        System::to_lower(model_local);

        if (model_local.find("sl75") != std::string::npos) {
            sl75_bober_kurwa = true;
        } else if (data.get_size() > 0x04000000) {
            sl75_bober_kurwa = true;
        }
    }

    if (platform != Platform::UNK) {
        bool borken_imei = false;
        bool broken_model = false;

        if (imei.size() != 15) {
            borken_imei = true;
        } else {
            std::for_each(imei.cbegin(), imei.cend(), [&](char c) {
                if (!isprint(c)) {
                    borken_imei = true;
                }
            });
        }

        std::for_each(model.cbegin(), model.cend(), [&](char c) {
            if (!isprint(c)) {
                broken_model = true;
            }
        });


        if (borken_imei) {
            Log::Logger::warn("Couldn't detect IMEI");
            Log::Logger::debug("IMEI broken: {}", imei);

            imei.clear();
        }

        if (broken_model) {
            Log::Logger::warn("Couldn't detect model");

            model = PlatformToString.at(platform);
        }
    }
}

void Partitions::old_search_partitions_egold_ce() {
    auto addresses   = find_pattern(pattern_egold, 0x0, false);

    std::map<uint32_t, Block::Header> headers;

    for (const auto &addr : addresses) {
        if ((addr & 0xFFF) != 0x80) {
            continue;
        }

        char *      ptr         = data.get_data().get() + addr + 2;
        uint32_t    block_addr  = addr & 0xFFFFFF00;
        char *      block_ptr   = data.get_data().get() + block_addr;

        Block::Header header;

        ptr = read_data<char>(header.name, ptr, 6);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_1), ptr, 1);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_2), ptr, 1);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_4), ptr, 1);
 
        size_t end_of_name = search_end(header.name, 6);

        if (!is_printable(header.name, end_of_name)) {
            continue;
        }

        if (headers.count(block_addr)) {
            throw Exception("Block with address {:08X} already exists", block_addr);
        }

        headers[block_addr] = header;
    }

    if (headers.size() >= 2) {
        auto block_1_it = headers.cbegin();
        auto block_2_it = block_1_it;

        block_2_it++;

        size_t block_1_addr = block_1_it->first;
        size_t block_2_addr = block_2_it->first;

        block_size = block_2_addr - block_1_addr;
    }

    Log::Logger::debug("Detected block size: {:08X}", block_size);

    for (const auto &pair : headers) {
        uint32_t    block_addr  = pair.first;
        const auto &header      = pair.second;
        char *      block_ptr   = data.get_data().get() + block_addr;

        std::string block_name(header.name);

        if (!partitions_map.count(block_name)) {
            partitions_map[block_name] = Partition(block_name);
        }

        partitions_map[block_name].add_block(Block(header, RawData(block_ptr, block_size), block_addr, block_size));

        Log::Logger::debug("Block: {:08X} {} {:04X} {:04X} {:04X}", block_addr, header.name, header.unknown_1, header.unknown_2, header.unknown_4);
    }
}

void Partitions::old_search_partitions_sgold_sgold2() {
    char *buf = data.get_data().get();

    for (size_t offset = 0; offset < data.get_size(); offset += block_size) {
        char *              ptr         = buf + offset;
        constexpr size_t    header_size = 4 + 2 + 2 + 8;

        if (is_empty(ptr, header_size)) {
            continue;
        }

        Block::Header header;

        ptr = read_data<char>(header.name, ptr, 8);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_1), ptr, 1);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_2), ptr, 1);
        ptr = read_data<uint32_t>(reinterpret_cast<char *>(&header.unknown_3), ptr, 1);

        if (header.unknown_3 != 0xFFFFFFF0) {
            continue;
        }

        if (header.unknown_2 != 0x0000 && header.unknown_3 != 0xFFFFFFF0) {
            continue;
        }

        size_t end_of_name = search_end(header.name, 8);

        if (end_of_name == 8) {
            continue;
        }

        if (!is_printable(header.name, end_of_name)) {
            continue;
        }

        header.name[end_of_name] = '\0';

        std::string block_name(header.name);
        std::vector<std::string> names = { "EEFULL", "EELITE", "FFS" };

        bool any_find = false;

        for (const auto &name : names) {
            if (block_name.find(name) != std::string::npos) {
                any_find = true;

                break;
            }
        }

        if (any_find == false) {
            continue;
        }

        if (!partitions_map.count(block_name)) {
            partitions_map[block_name] = Partition(block_name);
        }

        Log::Logger::debug("Name addr: {}, Addr: {:08X}, size {:08X}", block_name, offset, block_size * 2);

        partitions_map[block_name].add_block(Block(header, RawData(buf + offset, block_size * 2), offset, block_size * 2));

        // if (!blocks_map.count(block_name)) {
        //     blocks_map[block_name] = std::vector<Block>();
        // }

        // Block block;

        // block.header    = header;
        // block.offset    = offset;
        // block.count     = 2;
        // block.data      = RawData(buf + offset, block_size * block.count);
        // // block.size      = block_size * block.count;
        // // block.data      = std::shared_ptr<char[]>(new char[block.size]);

        // // ptr = buf + offset;
        // // memcpy(block.data.get(), ptr, block.size);

        // blocks_map[block_name].push_back(block);

        offset += block_size;
    }
}

void Partitions::old_search_partitions_sgold2_elka() {
    char *buf = data.get_data().get();

    for (size_t offset = 0; offset < data.get_size(); offset += block_size) {
        char *              ptr         = buf + offset;
        constexpr size_t    header_size = 4 + 2 + 2 + 8;

        if (is_empty(ptr, header_size)) {
            continue;
        }

        Block::Header header;

        ptr = read_data<char>(header.name, ptr + block_size - 32, 8);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_1), ptr, 1);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_2), ptr, 1);
        ptr = read_data<uint32_t>(reinterpret_cast<char *>(&header.unknown_3), ptr, 1);

        if (header.unknown_3 != 0xFFFFFFF0) {
            continue;
        }

        // Log::Logger::debug("Name: {}, Unk1: {:04X}, Unk2: {:04X}, Unk3: {:08X}", header.name, header.unknown_1, header.unknown_2, header.unknown_3);

        // if (header.unknown_2 != 0x0000 && header.unknown_3 != 0xFFFFFFF0) {
        //     continue;
        // }

        size_t end_of_name = search_end(header.name, 8);

        if (end_of_name == 8) {
            continue;
        }

        if (!is_printable(header.name, end_of_name)) {
            continue;
        }

        header.name[end_of_name] = '\0';

        std::string block_name(header.name);
        std::vector<std::string> names = { "EEFULL", "EELITE", "FFS" };

        bool any_find = false;

        for (const auto &name : names) {
            if (block_name.find(name) != std::string::npos) {
                any_find = true;

                break;
            }
        }

        if (any_find == false) {
            continue;
        }

        if (!partitions_map.count(block_name)) {
            partitions_map[block_name] = Partition(block_name);
        }

        uint32_t    block_count     = 4;
        uint32_t    block_offset    = offset - (block_size * (block_count - 1));

        partitions_map[block_name].add_block(Block(header, RawData(buf + block_offset, block_size * block_count), block_offset, block_size * block_count));
    }
}

std::vector<uint32_t> Partitions::find_pattern(const Patterns::Readable &pattern_readable, uint32_t start, bool break_first) {
    std::vector<uint32_t>       address_list;
    Patterns::Pattern<uint32_t> pattern(pattern_readable);

    address_list.reserve(1000);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = start; i < data.get_size() - pattern_readable.size(); i += 4) {
        uint32_t *data_ptr   = reinterpret_cast<uint32_t *>(data.get_data().get() + i);

        if (!pattern.match(data_ptr)) {
            continue;
        }

        address_list.push_back(i);

        Log::Logger::debug("Match addr: {:08X}", i);
        Log::Logger::debug("{}", pattern.to_string());

        std::string match_data;

        for (size_t j = 0; j < pattern_readable.size(); ++j) {
            match_data += fmt::format("{:08X} ", + *(data_ptr + j));
        }
        Log::Logger::debug(match_data);

        if (break_first) {
            break;
        }
    }

    auto end_time   = std::chrono::high_resolution_clock::now();
    auto diff_time  = end_time - start_time;

    Log::Logger::debug("Search end. Time: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(diff_time).count());

    return address_list;
}

};
};
