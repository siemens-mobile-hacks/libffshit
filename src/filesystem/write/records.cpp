#include "filesystem/write/records.h"

#include "ffshit/filesystem/ex.h"

#include <algorithm>
#include <cstring>

namespace FULLFLASH {
namespace Filesystem {
namespace Write {

Records::Ptr Records::build(Platform::Type platform, Partitions::Partitions::Ptr partitions, const std::string &partition_name) {
    switch (platform) {
        case Platform::Type::SGOLD:
        case Platform::Type::SGOLD2:        return std::make_unique<LinearRecords>(partitions, partition_name);
        case Platform::Type::SGOLD2_ELKA:   return std::make_unique<ElkaRecords>(partitions, partition_name);
        default: {
            throw Exception("Writing is not supported on this platform");
        }
    }
}

Records::Records(Partitions::Partitions::Ptr partitions, const std::string &partition_name) :
    image(partitions->get_data()),
    partition_name(partition_name),
    used(MAX_ID + 2),
    pending(MAX_ID + 2),
    excluded(MAX_ID + 2),
    free_hint(0),
    spare(SIZE_MAX),
    in_transaction(false) {

    const auto &partitions_map = partitions->get_partitions();

    if (!partitions_map.count(partition_name)) {
        throw Exception("Partition {} not found", partition_name);
    }

    for (const auto &block : partitions_map.at(partition_name).get_blocks()) {
        blocks.push_back({ block.get_addr(), block.get_size(), {}, 0, 0 });
    }
}

void Records::scan() {
    index.clear();

    std::fill(used.begin(), used.end(), false);
    std::fill(pending.begin(), pending.end(), false);
    std::fill(excluded.begin(), excluded.end(), false);

    free_hint = 0;

    for (size_t i = 0; i < blocks.size(); ++i) {
        scan_block(blocks[i]);
        index_block(i);
    }

    // The firmware's own records list ids: of the operations it logged, of the files it keeps
    // open. Whatever they mention is never handed out.
    for (uint32_t id = 1; id <= 5; ++id) {
        if (!contains(id)) {
            continue;
        }

        RawData data = read(id);

        for (size_t offset = 0; offset + 2 <= data.get_size(); offset += 2) {
            uint16_t value;

            data.read_type<uint16_t>(offset, &value);
            excluded[value] = true;
        }
    }

    // An erased block if there is one, else one whose records are all deleted
    if (spare == SIZE_MAX) {
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto &entries = blocks[i].entries;

            bool has_valid = std::any_of(entries.begin(), entries.end(), [](const Entry &entry) {
                return entry.flags == FLAGS_VALID;
            });

            if (has_valid) {
                continue;
            }

            if (entries.empty()) {
                spare = i;

                break;
            }

            if (spare == SIZE_MAX) {
                spare = i;
            }
        }
    }
}

void Records::index_block(size_t block_index) {
    const Block &block = blocks[block_index];

    for (size_t i = 0; i < block.entries.size(); ++i) {
        const Entry &entry = block.entries[i];

        if (entry.flags != FLAGS_VALID) {
            continue;
        }

        if (index.count(entry.id)) {
            throw Exception("Partition {} has two records with id {}. Broken filesystem? Not writing to it", partition_name, entry.id);
        }

        index[entry.id] = { block_index, i };
        mark_used(entry.id, true);
    }
}

bool Records::contains(uint32_t id) const {
    return index.count(id) != 0;
}

RawData Records::read(uint32_t id) const {
    auto it = index.find(id);

    if (it == index.end()) {
        throw Exception("Partition {}: record {} not found", partition_name, id);
    }

    const Block &block = blocks[it->second.first];

    return read_entry(block, block.entries[it->second.second]);
}

void Records::add(uint32_t id, const RawData &data) {
    if (contains(id)) {
        throw Exception("Partition {}: record {} exists already", partition_name, id);
    }

    size_t  block_index = find_block(data.get_size());
    Block & block       = blocks[block_index];

    touch(block_index);
    append(block, id, data);

    index[id] = { block_index, block.entries.size() - 1 };
    mark_used(id, true);
}

void Records::patch(uint32_t id, size_t offset, const std::vector<uint8_t> &bytes) {
    auto it = index.find(id);

    if (it == index.end()) {
        throw Exception("Partition {}: record {} not found", partition_name, id);
    }

    const Block &block = blocks[it->second.first];
    const Entry &entry = block.entries[it->second.second];

    if (offset + bytes.size() > entry.size) {
        throw Exception("Partition {}: patch of {} bytes at {} is beyond record {} of {} bytes", partition_name, bytes.size(), offset, id, entry.size);
    }

    touch(it->second.first);

    for (size_t i = 0; i < bytes.size(); ++i) {
        size_t  address = entry_address(block, entry, offset + i);
        uint8_t current;

        image.read_type<uint8_t>(address, &current);

        if ((current & bytes[i]) != bytes[i]) {
            throw Exception("Partition {}: changing record {} in place would set bits the flash cannot", partition_name, id);
        }

        write(address, reinterpret_cast<const char *>(&bytes[i]), 1);
    }
}

void Records::remove(uint32_t id) {
    auto it = index.find(id);

    if (it == index.end()) {
        throw Exception("Partition {}: record {} not found", partition_name, id);
    }

    Block &block = blocks[it->second.first];
    Entry &entry = block.entries[it->second.second];

    touch(it->second.first);
    write_u32(block.addr + entry.fit_offset, FLAGS_DELETED);

    entry.flags = FLAGS_DELETED;

    index.erase(it);
    mark_used(id, false);
}

uint32_t Records::allocate_pair(uint32_t min_id) {
    uint32_t id = std::max(min_id, free_hint);

    id += id & 1;

    while (taken(id) || taken(id + 1)) {
        if (id + 1 >= MAX_ID) {
            throw Exception("Partition {} has no free ids left", partition_name);
        }

        id += 2;
    }

    pending[id]     = true;
    pending[id + 1] = true;
    free_hint       = id + 2;

    return id;
}

bool Records::taken(uint32_t id) const {
    if (id > MAX_ID) {
        return true;
    }

    return used[id] || pending[id] || excluded[id];
}

void Records::mark_used(uint32_t id, bool is_used) {
    if (id > MAX_ID) {
        return;
    }

    used[id]    = is_used;
    pending[id] = false;

    if (!is_used && id < free_hint) {
        free_hint = id & ~1u;
    }
}

void Records::begin() {
    in_transaction = true;
    saved_blocks.clear();
}

void Records::commit() {
    in_transaction = false;
    saved_blocks.clear();

    std::fill(pending.begin(), pending.end(), false);
}

void Records::rollback() {
    for (const auto &pair : saved_blocks) {
        const RawData &saved = pair.second;

        image.write(blocks[pair.first].addr, saved.get_data().get(), saved.get_size());
    }

    in_transaction = false;
    saved_blocks.clear();

    scan();
}

void Records::touch(size_t block_index) {
    if (!in_transaction || saved_blocks.count(block_index)) {
        return;
    }

    const Block &block = blocks[block_index];

    saved_blocks.emplace(block_index, RawData(image, block.addr, block.size));
}

size_t Records::dead_space(const Block &block) const {
    size_t size = 0;

    for (const auto &entry : block.entries) {
        if (entry.flags != FLAGS_VALID) {
            size += entry.size + 16;
        }
    }

    return size;
}

size_t Records::find_block(uint32_t size) {
    // The block that fits it most tightly, so that the emptier blocks stay empty
    size_t      best        = SIZE_MAX;
    uint32_t    best_free   = UINT32_MAX;

    for (size_t i = 0; i < blocks.size(); ++i) {
        if (i == spare || !fits(blocks[i], size)) {
            continue;
        }

        uint32_t free = blocks[i].fit_next - blocks[i].data_end;

        if (free < best_free) {
            best        = i;
            best_free   = free;
        }
    }

    if (best != SIZE_MAX) {
        return best;
    }

    std::vector<size_t> candidates;

    for (size_t i = 0; i < blocks.size(); ++i) {
        if (i != spare && dead_space(blocks[i]) != 0) {
            candidates.push_back(i);
        }
    }

    std::stable_sort(candidates.begin(), candidates.end(), [this](size_t a, size_t b) {
        return dead_space(blocks[a]) > dead_space(blocks[b]);
    });

    for (size_t candidate : candidates) {
        compact(candidate);

        if (fits(blocks[candidate], size)) {
            return candidate;
        }
    }

    throw Exception("Not enough free space in partition {}", partition_name);
}

void Records::compact(size_t block_index) {
    Block &block = blocks[block_index];

    std::vector<std::pair<uint32_t, RawData>> valid;

    for (const auto &entry : block.entries) {
        if (entry.flags == FLAGS_VALID) {
            valid.emplace_back(entry.id, read_entry(block, entry));
        }
    }

    touch(block_index);
    erase(block);

    for (const auto &record : valid) {
        append(block, record.first, record.second);

        index[record.first] = { block_index, block.entries.size() - 1 };
    }
}

void Records::write(size_t address, const char *data, size_t size) {
    image.write(address, data, size);
}

void Records::write_u32(size_t address, uint32_t value) {
    write(address, reinterpret_cast<const char *>(&value), sizeof(value));
}

void Records::fill(size_t address, size_t size) {
    std::vector<char> erased(size, static_cast<char>(0xFF));

    write(address, erased.data(), size);
}

void Records::increment_erase_counter(size_t address) {
    uint16_t counter;

    image.read_type<uint16_t>(address, &counter);

    if (counter != 0xFFFF) {
        ++counter;
    }

    write(address, reinterpret_cast<const char *>(&counter), sizeof(counter));
}

// =========================================================================

static constexpr uint32_t LINEAR_HEADER_SIZE    = 16;
static constexpr uint32_t LINEAR_FIT_ENTRY_SIZE = 16;

LinearRecords::LinearRecords(Partitions::Partitions::Ptr partitions, const std::string &partition_name) :
    Records(partitions, partition_name) {

    scan();
}

void LinearRecords::scan_block(Block &block) const {
    block.entries.clear();
    block.data_end = LINEAR_HEADER_SIZE;

    uint32_t offset = block.size - LINEAR_FIT_ENTRY_SIZE;

    while (offset > 0) {
        Entry entry;

        entry.fit_offset = offset;

        image.read_type<uint32_t>(block.addr + offset + 0x0, &entry.flags);
        image.read_type<uint32_t>(block.addr + offset + 0x4, &entry.id);
        image.read_type<uint32_t>(block.addr + offset + 0x8, &entry.size);
        image.read_type<uint32_t>(block.addr + offset + 0xC, &entry.offset);

        if (entry.flags == FLAGS_FREE) {
            break;
        }

        block.entries.push_back(entry);

        if (entry.offset + entry.size <= block.size) {
            block.data_end = std::max(block.data_end, entry.offset + entry.size);
        }

        offset -= LINEAR_FIT_ENTRY_SIZE;
    }

    block.fit_next = offset;
}

RawData LinearRecords::read_entry(const Block &block, const Entry &entry) const {
    if (entry.size == 0) {
        return RawData();
    }

    return RawData(image, block.addr + entry.offset, entry.size);
}

size_t LinearRecords::entry_address(const Block &block, const Entry &entry, size_t offset) const {
    return block.addr + entry.offset + offset;
}

bool LinearRecords::fits(const Block &block, uint32_t size) const {
    // The entry, and the free entry that ends the FIT after it
    return block.data_end + size + LINEAR_FIT_ENTRY_SIZE <= block.fit_next;
}

void LinearRecords::append(Block &block, uint32_t id, const RawData &data) {
    uint32_t size = data.get_size();

    if (size) {
        write(block.addr + block.data_end, data.get_data().get(), size);
    }

    write_u32(block.addr + block.fit_next + 0x4, id);
    write_u32(block.addr + block.fit_next + 0x8, size);
    write_u32(block.addr + block.fit_next + 0xC, block.data_end);
    write_u32(block.addr + block.fit_next + 0x0, FLAGS_VALID);

    block.entries.push_back({ block.fit_next, FLAGS_VALID, id, size, block.data_end });

    block.data_end += size;
    block.fit_next -= LINEAR_FIT_ENTRY_SIZE;
}

void LinearRecords::erase(Block &block) {
    increment_erase_counter(block.addr + 8);
    fill(block.addr + LINEAR_HEADER_SIZE, block.size - LINEAR_HEADER_SIZE);

    block.entries.clear();
    block.data_end = LINEAR_HEADER_SIZE;
    block.fit_next = block.size - LINEAR_FIT_ENTRY_SIZE;
}

// =========================================================================

static constexpr uint32_t ELKA_SLOT_SIZE        = 32;
static constexpr uint32_t ELKA_SLOT_DATA_SIZE   = 16;
// The header takes the block's last slot, the FIT starts in the one below
static constexpr uint32_t ELKA_HEADER_OFFSET    = ELKA_SLOT_SIZE;
static constexpr uint32_t ELKA_FIT_OFFSET       = 2 * ELKA_SLOT_SIZE;
static constexpr uint32_t ELKA_INLINE_MAX       = 0x200;
static constexpr uint32_t ELKA_DATA_UNIT        = 0x400;
static constexpr uint32_t ELKA_RECORD_MAX       = 0x1000;

ElkaRecords::ElkaRecords(Partitions::Partitions::Ptr partitions, const std::string &partition_name) :
    Records(partitions, partition_name) {

    scan();
}

ElkaRecords::Kind ElkaRecords::kind(uint32_t size) {
    uint32_t tail = size & (ELKA_DATA_UNIT - 1);

    if (size <= ELKA_INLINE_MAX) {
        return Kind::INLINE;
    }

    if ((size & 0x1C00) && tail > 0 && tail <= ELKA_INLINE_MAX) {
        return Kind::SPLIT;
    }

    return Kind::DATA_AREA;
}

static uint32_t inline_slots_size(uint32_t size) {
    return (1 + (size + ELKA_SLOT_DATA_SIZE - 1) / ELKA_SLOT_DATA_SIZE) * ELKA_SLOT_SIZE;
}

uint32_t ElkaRecords::fit_size(uint32_t size) {
    switch (kind(size)) {
        case Kind::INLINE:      return inline_slots_size(size);
        case Kind::SPLIT:       return inline_slots_size(size & (ELKA_DATA_UNIT - 1));
        case Kind::DATA_AREA:
        default:                return ELKA_SLOT_SIZE;
    }
}

uint32_t ElkaRecords::data_area_size(uint32_t size) {
    switch (kind(size)) {
        case Kind::INLINE:      return 0;
        case Kind::SPLIT:       return size & ~(ELKA_DATA_UNIT - 1);
        case Kind::DATA_AREA:
        default:                return (size + ELKA_DATA_UNIT - 1) & ~(ELKA_DATA_UNIT - 1);
    }
}

void ElkaRecords::scan_block(Block &block) const {
    block.entries.clear();
    block.data_end = 0;

    uint32_t offset = block.size - ELKA_FIT_OFFSET;

    while (offset > 0) {
        Entry entry;

        entry.fit_offset = offset;

        image.read_type<uint32_t>(block.addr + offset + 0x0, &entry.flags);
        image.read_type<uint32_t>(block.addr + offset + 0x4, &entry.id);
        image.read_type<uint32_t>(block.addr + offset + 0x8, &entry.size);
        image.read_type<uint32_t>(block.addr + offset + 0xC, &entry.offset);

        if (entry.flags == FLAGS_FREE && entry.id == 0xFFFFFFFF && entry.size == 0xFFFFFFFF && entry.offset == 0xFFFFFFFF) {
            break;
        }

        block.entries.push_back(entry);

        // An inline record notes where the data area ended when it was written
        uint32_t data_end = entry.offset + data_area_size(entry.size);

        if (data_end <= offset) {
            block.data_end = std::max(block.data_end, data_end);
        }

        uint32_t step = fit_size(entry.size);

        if (step >= offset) {
            offset = 0;

            break;
        }

        offset -= step;
    }

    block.fit_next = offset;
}

size_t ElkaRecords::inline_address(const Block &block, uint32_t fit_offset, uint32_t size, size_t offset) const {
    // The slot right under the entry holds the last 16 bytes, the lowest one the first bytes,
    // aligned to the end of its 16
    size_t slot         = (size - 1 - offset) / ELKA_SLOT_DATA_SIZE;
    size_t slot_offset  = fit_offset - ELKA_SLOT_SIZE * (slot + 1);

    return block.addr + slot_offset + ELKA_SLOT_DATA_SIZE - (size - ELKA_SLOT_DATA_SIZE * slot - offset);
}

RawData ElkaRecords::read_inline(const Block &block, uint32_t fit_offset, uint32_t size) const {
    RawData data;

    // A slot's bytes are contiguous, and only the first slot's are fewer than 16
    for (size_t offset = 0; offset < size;) {
        size_t chunk = (size - offset) % ELKA_SLOT_DATA_SIZE ? (size - offset) % ELKA_SLOT_DATA_SIZE : ELKA_SLOT_DATA_SIZE;

        data.add(image.get_data().get() + inline_address(block, fit_offset, size, offset), chunk);

        offset += chunk;
    }

    return data;
}

void ElkaRecords::write_inline(const Block &block, uint32_t fit_offset, const char *data, uint32_t size) {
    for (size_t offset = 0; offset < size;) {
        size_t chunk = (size - offset) % ELKA_SLOT_DATA_SIZE ? (size - offset) % ELKA_SLOT_DATA_SIZE : ELKA_SLOT_DATA_SIZE;

        write(inline_address(block, fit_offset, size, offset), data + offset, chunk);

        offset += chunk;
    }
}

RawData ElkaRecords::read_entry(const Block &block, const Entry &entry) const {
    switch (kind(entry.size)) {
        case Kind::INLINE: {
            return read_inline(block, entry.fit_offset, entry.size);
        }

        case Kind::SPLIT: {
            uint32_t    head = entry.size & ~(ELKA_DATA_UNIT - 1);
            RawData     data(image, block.addr + entry.offset, head);

            data.add(read_inline(block, entry.fit_offset, entry.size - head));

            return data;
        }

        case Kind::DATA_AREA:
        default: {
            return RawData(image, block.addr + entry.offset, entry.size);
        }
    }
}

size_t ElkaRecords::entry_address(const Block &block, const Entry &entry, size_t offset) const {
    switch (kind(entry.size)) {
        case Kind::INLINE: {
            return inline_address(block, entry.fit_offset, entry.size, offset);
        }

        case Kind::SPLIT: {
            uint32_t head = entry.size & ~(ELKA_DATA_UNIT - 1);

            if (offset < head) {
                return block.addr + entry.offset + offset;
            }

            return inline_address(block, entry.fit_offset, entry.size - head, offset - head);
        }

        case Kind::DATA_AREA:
        default: {
            return block.addr + entry.offset + offset;
        }
    }
}

bool ElkaRecords::fits(const Block &block, uint32_t size) const {
    if (size > ELKA_RECORD_MAX) {
        return false;
    }

    uint32_t fit = fit_size(size);

    // The free entry that ends the FIT after it must stay above the data area
    return fit <= block.fit_next && block.fit_next - fit >= block.data_end + data_area_size(size);
}

void ElkaRecords::append(Block &block, uint32_t id, const RawData &data) {
    uint32_t        size    = data.get_size();
    const char *    bytes   = size ? data.get_data().get() : nullptr;

    switch (kind(size)) {
        case Kind::INLINE: {
            write_inline(block, block.fit_next, bytes, size);

            break;
        }

        case Kind::SPLIT: {
            uint32_t head = size & ~(ELKA_DATA_UNIT - 1);

            write(block.addr + block.data_end, bytes, head);
            write_inline(block, block.fit_next, bytes + head, size - head);

            break;
        }

        case Kind::DATA_AREA: {
            write(block.addr + block.data_end, bytes, size);

            break;
        }
    }

    write_u32(block.addr + block.fit_next + 0x4, id);
    write_u32(block.addr + block.fit_next + 0x8, size);
    write_u32(block.addr + block.fit_next + 0xC, block.data_end);
    write_u32(block.addr + block.fit_next + 0x0, FLAGS_VALID);

    block.entries.push_back({ block.fit_next, FLAGS_VALID, id, size, block.data_end });

    block.data_end += data_area_size(size);
    block.fit_next -= fit_size(size);
}

void ElkaRecords::erase(Block &block) {
    increment_erase_counter(block.addr + block.size - ELKA_HEADER_OFFSET + 8);
    fill(block.addr, block.size - ELKA_HEADER_OFFSET);

    block.entries.clear();
    block.data_end = 0;
    block.fit_next = block.size - ELKA_FIT_OFFSET;
}

};
};
};
