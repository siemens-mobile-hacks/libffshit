/*
    Thanks to:

        Partitions search algorithm:
            Azq2, marry_on_me, Feyman

        SGOLD/SGOLD2/ELKA partitions table start address:
            Feyman

        FAT Timestamp:
            perk11

        EGOLD Disk resizing patches
        (Their patches assisted in the analysis of the disk partition table):
            kay
            AlexSid
            SiNgle
            Chaos
            avkiev
            Baloo

        Smelter tool:
            avkiev

        Testing:
            perk11
            Feyman
            FIL
            maximuservice

    ♥♥♥ Love you guys ♥♥♥
*/

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

static const Patterns::Readable pattern_sg_nsg_table_pointer {
    "4F 54 50 00",
    "?? ?? ?? A0"
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

// 00426A3A: 01 00 10 2A 89 01 

static const Patterns::Readable pattern_egold_table_pointer {
    "??", "00", "00", "00",             // Количество записей
    "0?", "00", "??", "??", "??", "0?"  // Адрес таблицы
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

Partitions::Partitions(const RawData& raw_data, Detector::Ptr detector, bool old_search_algorithm, uint32_t search_start_addr) : data(raw_data) {
    this->detector      = detector;
    this->fs_platform   = detector->get_platform();

    search_partitions(old_search_algorithm, search_start_addr);

    Log::Logger::debug("Found {} FFS partitions", partitions_map.size());

    for (const auto &pair : partitions_map) {
        Log::Logger::debug("  {:8s} {}", pair.first, pair.second.get_blocks().size());
    }
}

const Partitions::Map &Partitions::get_partitions() const {
    return partitions_map;
}

const RawData &Partitions::get_data() const {
    return data;
}

const Detector::Ptr &Partitions::get_detector() const {
    return detector;
}

Platform Partitions::get_fs_platform() const {
    return fs_platform;
}

void Partitions::search_partitions(bool old_search_algorithm, uint32_t start_addr) {
    auto old_search = [&]() {
        switch (detector->get_platform()) {
            case Platform::EGOLD_CE:    block_size = 0x10000; old_search_partitions_egold_ce(); break;
            case Platform::SGOLD:
            case Platform::SGOLD2:      block_size = 0x10000; old_search_partitions_sgold_sgold2(); break;
            case Platform::SGOLD2_ELKA: block_size = 0x10000; old_search_partitions_sgold2_elka(); break;
            default: throw Exception("Couldn't detect fullflash platform");
        }
    };

    auto new_search = [&]() {
        switch (detector->get_platform()) {
            case Platform::EGOLD_CE:    search_partitions_egold(start_addr); break;
            case Platform::SGOLD:       search_partitions_sgold(start_addr); break;
            case Platform::SGOLD2:      search_partitions_sgold2(start_addr); break;
            case Platform::SGOLD2_ELKA: search_partitions_sgold2_elka(start_addr); break;

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

    inspect();

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

// Маппинг блоков на M55 fw91

// ========== FFS ==========

// Таблица указателей на адреса непосредственно блоков диска:
// Указатели прямые, просто - база
// 00426834: 00 00 D0 00 40 00 
// 0042683A: 00 00 D1 00 40 00 
// 00426840: 00 00 D2 00 40 00 
// 00426846: 00 00 D3 00 40 00 
// 0042684C: 00 00 D4 00 40 00 
// 00426852: 00 00 D5 00 40 00 
// 00426858: 00 00 D6 00 40 00 
// 0042685E: 00 00 D7 00 40 00 
// 00426864: 00 00 D8 00 40 00 
// 0042686A: 00 00 D9 00 40 00 
// 00426870: 00 00 DA 00 40 00 
// 00426876: 00 00 DB 00 40 00 
// 0042687C: 00 00 DC 00 40 00 
// 00426882: 00 00 DD 00 40 00 
// 00426888: 00 00 DE 00 40 00 
// 0042688E: 00 00 DF 00 40 00 
// 00426894: 00 00 E0 00 40 00 
// 0042689A: 00 00 E1 00 40 00 
// 004268A0: 00 00 E2 00 40 00 
// 004268A6: 00 00 E3 00 40 00 
// 004268AC: 00 00 E4 00 40 00 
// 004268B2: 00 00 E5 00 40 00 
// 004268B8: 00 00 E6 00 40 00 
// 004268BE: 00 00 E7 00 40 00 
// 004268C4: 00 00 E8 00 40 00 
// 004268CA: 00 00 E9 00 40 00 
// 004268D0: 00 00 EA 00 40 00 
// 004268D6: 00 00 EB 00 40 00 
// 004268DC: 00 00 EC 00 40 00 
// 004268E2: 00 00 ED 00 40 00 
// 004268E8: 00 00 EE 00 40 00 
// 004268EE: 00 00 EF 00 40 00 

// Таблица указатели на адреса блоков и их количество:
// 004268F4: 01 00 34 28 89 01 
// 004268FA: 01 00 3A 28 89 01 
// 00426900: 01 00 40 28 89 01 
// 00426906: 01 00 46 28 89 01 
// 0042690C: 01 00 4C 28 89 01 
// 00426912: 01 00 52 28 89 01 
// 00426918: 01 00 58 28 89 01 
// 0042691E: 01 00 5E 28 89 01 
// 00426924: 01 00 64 28 89 01 
// 0042692A: 01 00 6A 28 89 01 
// 00426930: 01 00 70 28 89 01 
// 00426936: 01 00 76 28 89 01 
// 0042693C: 01 00 7C 28 89 01 
// 00426942: 01 00 82 28 89 01 
// 00426948: 01 00 88 28 89 01 
// 0042694E: 01 00 8E 28 89 01 
// 00426954: 01 00 94 28 89 01    
// 0042695A: 01 00 9A 28 89 01 
// 00426960: 01 00 A0 28 89 01 
// 00426966: 01 00 A6 28 89 01 
// 0042696C: 01 00 AC 28 89 01 
// 00426972: 01 00 B2 28 89 01 
// 00426978: 01 00 B8 28 89 01 
// 0042697E: 01 00 BE 28 89 01 
// 00426984: 01 00 C4 28 89 01 	
// 0042698A: 01 00 CA 28 89 01 
// 00426990: 01 00 D0 28 89 01 
// 00426996: 01 00 D6 28 89 01 
// 0042699C: 01 00 DC 28 89 01 
// 004269A2: 01 00 E2 28 89 01 
// 004269A8: 01 00 E8 28 89 01 
// 004269AE: 01 00 EE 28 89 01 

// Указатель на начало таблицы:
// 004269B4:       20 00 00 00 // Количество записей
// 004269B8: 01 00 F4 28 89 01

// ========== FFS_B ==========

// Таблица указателей на адреса непосредственно блоков диска:
// Указатели прямые, просто - база
// 004269EC: 00 00 A8 00 40 00 
// 004269F2: 00 00 A9 00 40 00 
// 004269F8: 00 00 AA 00 40 00 
// 004269FE: 00 00 AB 00 40 00 
// 00426A04: 00 00 AC 00 40 00 
// 00426A0A: 00 00 AD 00 40 00 
// 00426A10: 00 00 AE 00 40 00 

// Таблица указатели на адреса блоков и их количество:
// 00426A16: 01 00 EC 29 89 01
// 00426A1C: 01 00 F2 29 89 01 
// 00426A22: 01 00 F8 29 89 01 
// 00426A28: 01 00 FE 29 89 01 
// 00426A2E: 01 00 04 2A 89 01 
// 00426A34: 01 00 0A 2A 89 01 
// 00426A3A: 01 00 10 2A 89 01 

// Указатель на начало таблицы:
// 00426A40:       07 00 00 00 // Количество записей
// 00426A44: 01 00 16 2A 89 01 

static uint32_t segment_to_page(uint32_t segment_addr) {
    uint16_t    segment         = segment_addr >> 16;
    uint16_t    segment_offset  = segment_addr & 0xFFFF;

    return ((segment * 0x4000) + segment_offset);
}

static uint32_t page_to_segment(uint32_t page_addr) {
    uint16_t    segment_offset  = page_addr & 0x3FFF;
    uint16_t    segment = (page_addr - segment_offset) / 0x4000;

    return static_cast<uint32_t>(segment) << 16 | static_cast<uint32_t>(segment_offset);
}

bool Partitions::search_partitions_egold(uint32_t start_addr) {
    uint32_t base_address = detector->get_base_address();

    Log::Logger::debug("EGOLD Base address: {:08X}", base_address);

    Log::Logger::debug("Searching partitions from 0x{:08X}", start_addr);

    auto addresses = find_pattern8(pattern_egold_table_pointer, start_addr, false);

    Log::Logger::debug("Found {} matches", addresses.size());

    if (!addresses.size()) {
        return false;
    }

    std::reverse(addresses.begin(), addresses.end());

    typedef struct {
        uint32_t    records_count;
        uint16_t    blocks_count;
        uint32_t    offset;
    } Table;

    std::vector<Table> tables;

    auto validate_table_start = [this, base_address](size_t offset, uint32_t records_count) -> bool {
        for (size_t i = 0; i < records_count; ++i) {
            uint16_t    blocks_count;
            uint32_t    block_segment_addr;

            data.read_type<uint16_t>(offset,     &blocks_count, 1);
            data.read_type<uint32_t>(offset + 2, &block_segment_addr, 1);

            // uint16_t    segment         = block_segment_addr >> 16;
            // uint16_t    segment_offset  = block_segment_addr & 0xFFFF;

            // size_t      ff_offset   = ((segment * 0x4000) + segment_offset) - base_address;

            size_t ff_offset = segment_to_page(block_segment_addr) - base_address;

            if (ff_offset >= data.get_size()) {
                return false;
            }

            if (ff_offset == 0) {
                return false;
            }

            uint32_t    block_addr;
            uint16_t    wtf;

            data.read_type<uint32_t>(ff_offset,        &block_addr, 1);
            data.read_type<uint16_t>(ff_offset + 4,    &wtf, 1);

            if (wtf > 0x80) {
                return false;
            }
            
            block_addr -= base_address;

            if ((block_addr & 0x0FFF) != 0) {
                return false;
            }
            
            size_t rd_header_offset = (block_addr + 2) | 0x80;

            if (rd_header_offset + 12 >= data.get_size()) {
                return false;
            }
            
            Block::Header header;
                        
            data.read<char>(rd_header_offset, reinterpret_cast<char *>(header.name), 6);
            data.read<uint16_t>(rd_header_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
            data.read<uint16_t>(rd_header_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
            data.read<uint16_t>(rd_header_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

            std::string block_name(header.name);

            if (!is_printable(block_name.data(), block_name.size())) {
                return false;
            }

            offset += 6;
        }

        return true;
    };

    for (auto &addr : addresses) {
        uint32_t    records_count;
        uint16_t    blocks_count;
        uint32_t    block_segment_addr;

        size_t offset = addr;

        data.read<uint32_t>(offset, reinterpret_cast<char *>(&records_count), 1);
        data.read<uint16_t>(offset, reinterpret_cast<char *>(&blocks_count), 1);
        data.read<uint32_t>(offset, reinterpret_cast<char *>(&block_segment_addr), 1);

        if (records_count == 0) {
            continue;
        }

        if (blocks_count == 0) {
            continue;
        }

        if (blocks_count > 4) {
            continue;
        }

        // uint16_t    segment         = block_segment_addr >> 16;
        // uint16_t    segment_offset  = block_segment_addr & 0xFFFF;

        // size_t      ff_offset   = ((segment * 0x4000) + segment_offset) - base_address;

        size_t  ff_offset = segment_to_page(block_segment_addr) - base_address;

        if (ff_offset >= data.get_size()) {
            continue;
        }

        if (ff_offset == 0) {
            continue;
        }

        bool is_table = validate_table_start(ff_offset, records_count);

        if (!is_table) {
            continue;
        }

        Log::Logger::debug("{:08X} {:08X} {:08X}: Records count: {:08X}, Blocks count: {:04X}, Segment addr: {:08X}, Page addr: {:08X} {:08X}",  
            addr, 
            addr + base_address,
            page_to_segment(addr + base_address),
            records_count,
            blocks_count,
            block_segment_addr, 
            ff_offset + base_address,
            ff_offset);

        Table table_record;

        table_record.blocks_count   = blocks_count;
        table_record.offset         = ff_offset;
        table_record.records_count  = records_count;

        bool skip = false;

        for (const auto &table : tables) {
            if (table.offset == table_record.offset) {
                Log::Logger::debug("Dupicate table. A60? Skip");

                skip = true;
            }
        }

        if (skip) {
            continue;
        }

        tables.push_back(table_record);
    }

    for (const auto &table : tables) {
        size_t offset = table.offset;

        Log::Logger::debug("Table:     Page addr: {:08X} -> {:08X}, Segment addr: {:08X}, Records: ", 
            table.offset + base_address,
            table.offset, 
            page_to_segment(table.offset + base_address),
            table.records_count);

        for (size_t i = 0; i < table.records_count; ++i) {
            uint16_t    blocks_count;
            uint32_t    block_segment_addr;

            data.read_type<uint16_t>(offset,     &blocks_count, 1);
            data.read_type<uint32_t>(offset + 2, &block_segment_addr, 1);

            // uint16_t segment         = block_segment_addr >> 16;
            // uint16_t segment_offset  = block_segment_addr & 0xFFFF;

            // size_t ff_offset   = ((segment * 0x4000) + segment_offset) - base_address;
            size_t ff_offset = segment_to_page(block_segment_addr) - base_address;

            Log::Logger::debug("  Segment addr: {:08X}, Page addr: {:08X} -> {:08X}", block_segment_addr, ff_offset + base_address, ff_offset);

            uint32_t    block_addr;
            uint16_t    wtf;

            data.read_type<uint32_t>(ff_offset,        &block_addr, 1);
            data.read_type<uint16_t>(ff_offset + 4,    &wtf, 1);
    
            block_addr -= base_address;
            
            size_t rd_header_offset = (block_addr + 2) | 0x80;
            
            Block::Header header;

            data.read<char>(rd_header_offset, reinterpret_cast<char *>(header.name), 6);
            data.read<uint16_t>(rd_header_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
            data.read<uint16_t>(rd_header_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
            data.read<uint16_t>(rd_header_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

            std::string block_name(header.name);

            Log::Logger::debug("    {:6s} Block addr: {:08X} -> {:08X} WTF: {:04X}", block_name, block_addr + base_address, block_addr, wtf);

            // Пока нужны только FFS
            if (block_name.find("FFS") != std::string::npos) {
                if (!partitions_map.count(block_name)) {
                    partitions_map[block_name] = Partition(block_name);
                }

                size_t  single_block_size   = 65536;

                if (wtf == 0x80) { // New EGOLD
                    single_block_size *= 2;
                }

                size_t  block_data_size     = single_block_size * blocks_count;

                RawData block_data;

                for (uint16_t i = 0; i < blocks_count; ++i) {
                    RawData tmp(data, block_addr + (single_block_size * i), single_block_size);

                    block_data.add(tmp);
                }

                partitions_map[block_name].add_block(Block(header, block_data, block_addr & 0xFFFFFF00, block_data_size));
            }

            offset += 6;
        }
    }

    return true;
}

bool Partitions::search_partitions_sgold(uint32_t start_addr) {
    Log::Logger::debug("Searching partitions from 0x{:08X}", start_addr);

    auto addresses = find_pattern(pattern_sg_nsg_table_pointer, start_addr, false);

    if (!addresses.size()) {
        return false;
    }

    for (auto &addr : addresses) {
        Log::Logger::debug("Pattern find, addr: {:08X}", addr);

        char        header[4];
        uint32_t    table_start_addr;

        data.read_type<char>(addr, header, 4);
        data.read_type<uint32_t>(addr + 4, &table_start_addr, 1);

        table_start_addr &= FF_ADDRESS_MASK;

        if (!match_pattern(pattern_sg, table_start_addr)) {
            Log::Logger::warn("Partitions table at address: {:08X} doesn't match ad SGOLD table pattern. Skip.", table_start_addr);

            continue;
        }

        Log::Logger::debug("{} {:08X}", header, table_start_addr);

        size_t struct_size = 0x2C;

        for (size_t offset = table_start_addr; offset < table_start_addr + 64 * struct_size; offset += struct_size) {
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

                // if (!partitions_map.count(partition_name)) {
                //     partitions_map[partition_name] = Partition(partition_name);
                // }

                if (partition_name.find("FFS") != std::string::npos) {
                    if (!partitions_map.count(partition_name)) {
                        partitions_map[partition_name] = Partition(partition_name);
                    }

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
                    // На самом деле, мне нужны только FFS, пока
                    // Block::Header header;

                    // // ну пусть будет
                    // size_t  block_rd_offset = masked_block_addr;

                    // data.read<char>(block_rd_offset, header.name, 8);
                    // data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
                    // data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
                    // data.read<uint32_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

                    // partitions_map[partition_name].add_block(
                    //     Block(  header, 
                    //             RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                    //             masked_block_addr, 
                    //             masked_block_size)
                    // );

                }
            }
        }

        if (!partitions_map.empty()) {
            break;
        }

        // if (!partitions_map.empty()) {
        //     bool ffs_found = false;

        //     for (const auto &pair : partitions_map) {
        //         if (pair.first.find("FFS") != std::string::npos) {
        //             ffs_found = true;

        //             break;
        //         }
        //     }

        //     if (ffs_found) {
        //         break;
        //     }

        //     partitions_map.clear();
        // }
    }

    return true;
}

bool Partitions::search_partitions_sgold2(uint32_t start_addr) {
    Log::Logger::debug("Searching partitions from 0x{:08X}", start_addr);

    auto address_list    = find_pattern(pattern_sg_nsg_table_pointer, start_addr, false);
    
    if (detector->is_sl75()) {
        Log::Logger::warn("SL75 ja pierdole!");
    }

    if (!address_list.size()) {
        return false;
    }

    for (auto &addr : address_list) {
        Log::Logger::debug("Pattern find, addr: {:08X}", addr);

        char        header[4];
        uint32_t    table_start_addr;

        data.read_type<char>(addr, header, 4);
        data.read_type<uint32_t>(addr + 4, &table_start_addr, 1);

        table_start_addr &= FF_ADDRESS_MASK;

        Log::Logger::debug("{} {:08X}", header, table_start_addr);

        if (!match_pattern(pattern_nsg, table_start_addr)) {
            bool is_sgold = false;

            if (match_pattern(pattern_sg, table_start_addr)) {
                Log::Logger::warn("Partitions table at address: {:08X} doesn't match as SGOLD2 table pattern.", table_start_addr);
                Log::Logger::warn("Detected platform SGOLD2, but partitions table format matched ad SGOLD. Using SGOLD partitions search. FS Platform overrided.");

                fs_platform = Platform::SGOLD;

                return search_partitions_sgold(start_addr);
            }
            Log::Logger::warn("Partitions table at address: {:08X} doesn't match as SGOLD2 table pattern. Skip.", table_start_addr);

            continue;
        }

        size_t struct_size = 0x34;

        for (size_t offset = table_start_addr; offset < table_start_addr + 64 * struct_size; offset += struct_size) {
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

                if (((block_addr & 0xFF000000) > 0xA2000000) && detector->is_sl75()) {
                    uint32_t shit = 0xA4000000 - 0xA2000000;
                    Log::Logger::debug("  JA PIERDOLE! {:08X} -> {:08X}", block_addr, block_addr - shit);

                    block_addr -= shit;
                }

                uint32_t masked_block_addr = block_addr & FF_ADDRESS_MASK;
                uint32_t masked_block_size = block_size & FF_ADDRESS_MASK;

                Log::Logger::debug("  Block:        Name: {}, Addr: {:08X}, size: {:08X}, table: {:08X}", partition_name, masked_block_addr, masked_block_size, table_addr + i);

                if (partition_name.find("FFS") != std::string::npos) {
                    if (!partitions_map.count(partition_name)) {
                        partitions_map[partition_name] = Partition(partition_name);
                    }

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
                    // На самом деле, мне нужны только FFS, пока
                    // Block::Header header;

                    // // ну пусть будет
                    // size_t  block_rd_offset = masked_block_addr;

                    // data.read<char>(block_rd_offset, header.name, 8);
                    // data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
                    // data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
                    // data.read<uint32_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

                    // partitions_map[partition_name].add_block(
                    //     Block(  header, 
                    //             RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                    //             masked_block_addr, 
                    //             masked_block_size)
                    // );
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
    Log::Logger::debug("Searching partitions from 0x{:08X}", start_addr);

    // auto address_list    = find_pattern(pattern_nsg);
    auto address_list    = find_pattern(pattern_sg_nsg_table_pointer, start_addr, false);
    
    if (detector->is_sl75()) {
        Log::Logger::warn("SL75 ja pierdole!");
    }

    if (!address_list.size()) {
        return false;
    }

    for (auto addr : address_list) {
        Log::Logger::debug("Pattern find, addr: {:08X}", addr);

        char        header[4];
        uint32_t    table_start_addr;

        data.read_type<char>(addr, header, 4);
        data.read_type<uint32_t>(addr + 4, &table_start_addr, 1);

        table_start_addr &= FF_ADDRESS_MASK;

        Log::Logger::debug("{} {:08X}", header, table_start_addr);

        size_t struct_size = 0x34;

        for (size_t offset = table_start_addr; offset < table_start_addr + 64 * struct_size; offset += struct_size) {
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

                uint32_t masked_block_addr = block_addr & FF_ADDRESS_MASK;
                uint32_t masked_block_size = block_size & FF_ADDRESS_MASK;

                Log::Logger::debug("  Block:        Name: {}, Addr: {:08X}, size: {:08X}, table: {:08X}", partition_name, masked_block_addr, masked_block_size, table_addr + i);

                if (partition_name.find("FFS") != std::string::npos) {
                    if (!partitions_map.count(partition_name)) {
                        partitions_map[partition_name] = Partition(partition_name);
                    }

                    Block::Header header;

                    size_t  block_rd_offset = masked_block_addr + masked_block_size - 0x20;

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
                    // На самом деле, мне нужны только FFS, пока
                    // Block::Header header;

                    // // ну пусть будет
                    // size_t  block_rd_offset = masked_block_addr;

                    // data.read<char>(block_rd_offset, header.name, 8);
                    // data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_1), 1);
                    // data.read<uint16_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_2), 1);
                    // data.read<uint32_t>(block_rd_offset, reinterpret_cast<char *>(&header.unknown_3), 1);

                    // partitions_map[partition_name].add_block(
                    //     Block(  header, 
                    //             RawData(data.get_data().get() + masked_block_addr, masked_block_size), 
                    //             masked_block_addr, 
                    //             masked_block_size)
                    // );
                }
            }
        }

        if (!partitions_map.empty()) {
            break;
        }
    }

    return true;
}

void Partitions::old_search_partitions_egold_ce() {
    auto addresses   = find_pattern(pattern_egold, 0x0, false);

    tsl::ordered_map<uint32_t, Block::Header> headers;

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

        std::string part_name(header.name);

        if (part_name.find("FFS") == std::string::npos) {
            continue;
        }

        if (headers.count(block_addr)) {
            throw Exception("Block with address {:08X} already exists", block_addr);
        }

        headers[block_addr] = header;

        Log::Logger::debug("Block: {:08X} {} {:04X} {:04X} {:04X}", block_addr, header.name, header.unknown_1, header.unknown_2, header.unknown_4);

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

        if (block_addr + block_size > data.get_size()) {
            Log::Logger::debug("WTF? {:08X} {}", block_addr, block_size);
            continue;
        }

        RawData block_data(block_ptr, block_size);

        partitions_map[block_name].add_block(Block(header, block_data, block_addr, block_size));

        // Log::Logger::debug("Block: {:08X} {} {:04X} {:04X} {:04X}", block_addr, header.name, header.unknown_1, header.unknown_2, header.unknown_4);
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

        // На самом деле, мне нужны только FFS, пока
        // std::vector<std::string> names = { "EEFULL", "EELITE", "FFS" };
        std::vector<std::string> names = { "FFS" };

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

        // На самом деле, мне нужны только FFS, пока
        // std::vector<std::string> names = { "EEFULL", "EELITE", "FFS" };
        std::vector<std::string> names = { "FFS" };

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

void Partitions::inspect() {
    for (auto iter = partitions_map.begin(); iter != partitions_map.end();) {
        auto &pair = *iter;

        if (pair.second.get_blocks().size() == 0) {
            Log::Logger::warn("Partition {} has 0 blocks. Removed from part. map", pair.first);
            partitions_map.erase(iter++);
        } else {
            ++iter;
        }
    }
}

std::vector<uint32_t> Partitions::find_pattern8(const Patterns::Readable &pattern_readable, uint32_t start, bool break_first) {
    std::vector<uint32_t>       address_list;
    Patterns::Pattern<uint8_t>  pattern(pattern_readable);

    Log::Logger::debug("Searching pattern: {}", pattern.to_string());

    address_list.reserve(1000);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = start; i < data.get_size() - pattern_readable.size(); i += 2) {
        uint8_t *data_ptr   = reinterpret_cast<uint8_t *>(data.get_data().get() + i);

        if (!pattern.match(data_ptr)) {
            continue;
        }

        address_list.push_back(i);

        // std::string match_data;

        // for (size_t j = 0; j < pattern_readable.size(); ++j) {
        //     uint8_t value = *(data_ptr + j);

        //     match_data += fmt::format("{:02X} ", value);
        // }

        // Log::Logger::debug("Match addr: {:08X}", i);
        // Log::Logger::debug("{}", pattern.to_string());
        // Log::Logger::debug(match_data);

        if (break_first) {
            break;
        }
    }

    auto end_time   = std::chrono::high_resolution_clock::now();
    auto diff_time  = end_time - start_time;

    Log::Logger::debug("Search end. Time: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(diff_time).count());

    return address_list;

}

std::vector<uint32_t> Partitions::find_pattern(const Patterns::Readable &pattern_readable, uint32_t start, bool break_first) {
    std::vector<uint32_t>       address_list;
    Patterns::Pattern<uint32_t> pattern(pattern_readable);

    Log::Logger::debug("Searching pattern: {}", pattern.to_string());

    address_list.reserve(1000);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = start; i < data.get_size() - pattern_readable.size(); i += 4) {
        uint32_t *data_ptr   = reinterpret_cast<uint32_t *>(data.get_data().get() + i);

        if (!pattern.match(data_ptr)) {
            continue;
        }

        address_list.push_back(i);

        // std::string match_data;

        // for (size_t j = 0; j < pattern_readable.size(); ++j) {
        //     match_data += fmt::format("{:08X} ", + *(data_ptr + j));
        // }

        // Log::Logger::debug("Match addr: {:08X}", i);
        // Log::Logger::debug("{}", pattern.to_string());
        // Log::Logger::debug(match_data);

        if (break_first) {
            break;
        }
    }

    auto end_time   = std::chrono::high_resolution_clock::now();
    auto diff_time  = end_time - start_time;

    Log::Logger::debug("Search end. Time: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(diff_time).count());

    return address_list;
}

bool Partitions::match_pattern(const Patterns::Readable &pattern_readable, uint32_t addr) {
    Patterns::Pattern<uint32_t> pattern(pattern_readable);

    uint32_t *data_ptr = reinterpret_cast<uint32_t *>(data.get_data().get() + addr);

    return pattern.match(data_ptr);
}


};
};
