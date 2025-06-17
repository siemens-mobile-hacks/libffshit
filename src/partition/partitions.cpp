/* =================================================================
   |                                                               |
   |  Thanks to Azq2, marry_on_me for partitions search algorithm  |
   |                                                               |
   |                   ♡♡♡ Love you guys ♡♡♡                       |
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

static constexpr size_t     BC65_BC75_OFFSET    = 0x870;
static constexpr size_t     BC85_OFFSET         = 0xC70;

static constexpr size_t     X65_MODEL_OFFSET    = 0x210;
static constexpr size_t     X75_MODEL_OFFSET    = 0x210;
static constexpr size_t     X85_MODEL_OFFSET    = 0x3E000;

static constexpr size_t     X65_IMEI_OFFSET     = 0x65C;
static constexpr size_t     X65_7X_IMEI_OFFSET  = 0x660;
static constexpr size_t     X75_IMEI_OFFSET     = 0x660;
static constexpr size_t     X85_IMEI_OFFSET     = 0x3E410;

static constexpr uint32_t   FF_ADDRESS_MASK     = 0x0FFFFFFF;

static constexpr uint32_t   X65_SEARCH_START_ADDR   =   0x00800000;
static constexpr uint32_t   X75_SEARCH_START_ADDR   =   0x004C0000;

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

Partitions::Partitions(std::string fullflash_path, bool old_search_alghoritm, bool search_from_addr, uint32_t search_start_addr) {
    sl75_bober_kurwa = false;
    
    std::ifstream file;

    file.open(fullflash_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw Exception("Couldn't open file '{}': {}\n", fullflash_path, std::string(strerror(errno)));
    }

    file.seekg(0, std::ios_base::end);
    size_t data_size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    this->data = RawData(file, 0, data_size);

    detect_platform();

    uint32_t start_addr = 0x0;

    if (old_search_alghoritm) {
        switch (platform) {
            case Platform::X65:
            case Platform::X75: block_size = 0x10000; old_search_blocks(); break;
            case Platform::X85: block_size = 0x10000; old_search_blocks_x85(); break;
            default: throw Exception("Couldn't detect fullflash platform");
        }
    } else {
        switch (platform) {
            case Platform::X65: {
                start_addr = X65_SEARCH_START_ADDR;

                if (search_from_addr) {
                    start_addr = search_start_addr;
                }

                search_partitions_x65(start_addr); 

                break;
            }
            case Platform::X75: {
                start_addr = X75_SEARCH_START_ADDR;

                search_partitions_x75(start_addr); 

                if (search_from_addr) {
                    start_addr = search_start_addr;
                }
                break;
            }
            case Platform::X85:  {
                Log::Logger::warn("New partitions search algorithm not implemented yet for X85");

                block_size = 0x10000; 

                old_search_blocks_x85(); 
                
                break;
            }
            default: throw Exception("Couldn't detect fullflash platform");
        }
    }

    Log::Logger::debug("Found {} partitions", partitions_map.size());

    for (const auto &pair : partitions_map) {
        Log::Logger::debug("  {:8s} {}", pair.first, pair.second.get_blocks().size());
    }

}

Partitions::Partitions(std::string fullflash_path, Platform platform, bool old_search_alghoritm, bool search_from_addr, uint32_t search_start_addr) {
    std::ifstream file;

    file.open(fullflash_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw Exception("Couldn't open file '{}': {}\n", fullflash_path, std::string(strerror(errno)));
    }

    file.seekg(0, std::ios_base::end);
    size_t data_size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    this->data      = RawData(file, 0, data_size);
    this->platform  = platform;

    uint32_t start_addr = 0x0;

    if (old_search_alghoritm) {
        switch (platform) {
            case Platform::X65:
            case Platform::X75: block_size = 0x10000; old_search_blocks(); break;
            case Platform::X85: block_size = 0x10000; old_search_blocks_x85(); break;
            default: throw Exception("Couldn't detect fullflash platform");
        }

    } else {
        switch (platform) {
            case Platform::X65: {
                start_addr = X65_SEARCH_START_ADDR;

                if (search_from_addr) {
                    start_addr = search_start_addr;
                }

                search_partitions_x65(start_addr); 

                break;
            }
            case Platform::X75: {
                start_addr = X75_SEARCH_START_ADDR;

                search_partitions_x75(start_addr); 

                if (search_from_addr) {
                    start_addr = search_start_addr;
                }
                break;
            }
            case Platform::X85:  {
                Log::Logger::warn("New partitions search algorithm not implemented yet for X85");

                block_size = 0x10000; 
                old_search_blocks_x85(); 
                
                break;
            }
            default: throw Exception("Couldn't detect fullflash platform");
        }
    }

    for (const auto &pair : partitions_map) {
        Log::Logger::debug("{:10s} {}", pair.first, pair.second.get_blocks().size());
    }
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

bool Partitions::search_partitions_x65(uint32_t start_addr) {
    Log::Logger::debug("Searching partitions");

    auto    addresses       = find_pattern(pattern_sg, start_addr, false);
    // auto    addresses       = find_pattern(pattern_sg);

    Log::Logger::debug("Search end");

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

            Log::Logger::debug("Name addr: {:08X}, Table addr: {:08X}, size {:08X}, {}", name_addr, table_addr, table_size, partition_name);

            for (size_t i = 0; i < table_size * 8; i += 8) {
                uint32_t block_addr;
                uint32_t block_size;

                data.read_type<uint32_t>(table_addr + i, &block_addr);
                data.read_type<uint32_t>(table_addr + i + 4, &block_size);

                uint32_t masked_block_addr = block_addr & FF_ADDRESS_MASK;
                uint32_t masked_block_size = block_size & FF_ADDRESS_MASK;

                Log::Logger::debug("  Name: {}, Addr: {:08X}, size: {:08X}, table: {:08X}", partition_name, masked_block_addr, masked_block_size, table_addr + i);

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

                    Log::Logger::debug("    {} {:08X} {:08X} {:08X}", header.name, header.unknown_1, header.unknown_2, header.unknown_3);

                    partitions_map[partition_name].add_block(
                        Block(  header, 
                                RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                                masked_block_addr, 
                                masked_block_size)
                    );
                } else {
                    Block::Header header;

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

bool Partitions::search_partitions_x75(uint32_t start_addr) {
    Log::Logger::debug("Searching partitions");

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

            Log::Logger::debug("Name addr: {:08X}, Table addr: {:08X}, size {:08X}, {}", name_addr, table_addr, table_size, partition_name);

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

                Log::Logger::debug("  Name: {}, Addr: {:08X}, size: {:08X}", partition_name, masked_block_addr, masked_block_size);

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

                    Log::Logger::debug("    {} {:08X} {:08X} {:08X}", header.name, header.unknown_1, header.unknown_2, header.unknown_3);

                    partitions_map[partition_name].add_block(
                        Block(  header, 
                                RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                                masked_block_addr, 
                                masked_block_size)
                    );
                } else {
                    Block::Header header;

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

bool Partitions::search_partitions_x85(uint32_t start_addr) {
    return false;
}

void Partitions::detect_platform() {
    std::string bc;

    data.read_string(BC65_BC75_OFFSET, bc, 1);

    if (bc == "BC65") {
        platform = Platform::X65;

        data.read_string(X65_MODEL_OFFSET, model);
        data.read_string(X65_IMEI_OFFSET, imei);

        if (imei.length() != 15) {
            imei.clear();
            data.read_string(X65_7X_IMEI_OFFSET, imei);
        }
    } else if (bc == "BC75") {
        platform = Platform::X75;

        data.read_string(X75_MODEL_OFFSET, model);
        data.read_string(X75_IMEI_OFFSET, imei);
    } else {
        bc.clear();

        data.read_string(BC85_OFFSET, bc, 1);

        if (bc == "BC85") {
            platform = Platform::X85;

            data.read_string(X85_MODEL_OFFSET, model);
            data.read_string(X85_IMEI_OFFSET, imei);

        } else {
            platform = Platform::UNK;
        }
    }

    if (platform == Platform::X75) {
        std::string model_local(model);

        System::to_lower(model_local);

        if (model_local.find("sl75") != std::string::npos) {
            sl75_bober_kurwa = true;
        } else if (data.get_size() > 0x04000000) {
            sl75_bober_kurwa = true;
        }
    }
}

void Partitions::old_search_blocks() {
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

void Partitions::old_search_blocks_x85() {
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

    for (size_t i = start; i < data.get_size(); i += 4) {
        uint32_t    *data_ptr   = reinterpret_cast<uint32_t *>(data.get_data().get() + i);

        bool match = true;

        if (!pattern.match(data_ptr)) {
            continue;
        }

        address_list.push_back(i);

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
