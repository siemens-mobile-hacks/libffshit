#include "filesystem/write/writer.h"

#include "ffshit/filesystem/ex.h"
#include "ffshit/filesystem/hash.h"

#include "filesystem/codepage.h"

#include <cstring>
#include <set>

namespace FULLFLASH {
namespace Filesystem {
namespace Write {

template<typename T>
static T read_value(const RawData &data, size_t offset) {
    T value;

    data.read_type<T>(offset, &value);

    return value;
}

template<typename T>
static void write_value(std::string &data, size_t offset, T value) {
    memcpy(&data[offset], &value, sizeof(value));
}

template<typename T>
static std::vector<uint8_t> value_bytes(T value) {
    std::vector<uint8_t> bytes(sizeof(value));

    memcpy(bytes.data(), &value, sizeof(value));

    return bytes;
}

static RawData to_raw_data(std::string data) {
    return RawData(&data[0], data.size());
}

static std::string join(const std::vector<std::string> &path) {
    std::string joined;

    for (const auto &name : path) {
        joined += (joined.empty() ? "" : "/") + name;
    }

    return joined;
}

// =========================================================================

Writer::Writer(Records::Ptr records) : records(std::move(records)), chunk_size(0) {
}

void Writer::init() {
    if (!records->contains(0)) {
        throw Exception("The filesystem has no configuration record. Not writing to it");
    }

    chunk_size = read_chunk_size(records->read(0));

    if (chunk_size < 256 || chunk_size > 4096 || (chunk_size & (chunk_size - 1))) {
        throw Exception("Unknown chunk size {}. Not writing to the filesystem", chunk_size);
    }

    if (!records->contains(root_id()) || !read_header(root_id()).is_directory()) {
        throw Exception("The filesystem has no root directory with id {}. A prototype's? Not writing to it", root_id());
    }
}

Records &Writer::get_records() {
    return *records;
}

bool Writer::same_name(const std::string &a, const std::string &b) const {
    try {
        return folded(to_stored(a)) == folded(to_stored(b));
    } catch (const FULLFLASH::BaseException &) {
        return a == b;
    }
}

Header Writer::read_header(uint32_t id) const {
    return decode_header(records->read(id));
}

Header Writer::resolve_directory(const std::vector<std::string> &path) const {
    Header directory = read_header(root_id());

    for (size_t i = 0; i < path.size(); ++i) {
        Found       found;
        std::string sub_path = join(std::vector<std::string>(path.begin(), path.begin() + i + 1));

        if (!find(directory, to_stored(path[i]), found)) {
            throw Exception("Directory {} not found", sub_path);
        }

        if (!found.header.is_directory()) {
            throw Exception("{} is not a directory", sub_path);
        }

        directory = found.header;
    }

    return directory;
}

Writer::Listing Writer::list(const Header &directory) const {
    Listing             listing;
    uint32_t            data_id = directory.data_id;
    uint32_t            next    = directory.next_part;
    std::set<uint32_t>  visited;

    listing.last_id = directory.id;

    while (true) {
        if (records->contains(data_id)) {
            RawData record = records->read(data_id);

            for (size_t offset = 0; offset + entry_size() <= record.get_size(); offset += entry_size()) {
                uint32_t id = entry_id(record, offset);

                if (id == none()) {
                    if (!listing.has_free) {
                        listing.has_free    = true;
                        listing.free        = { data_id, offset, id };
                    }
                } else if (id != 0) {
                    listing.entries.push_back({ data_id, offset, id });
                }
            }
        }

        // A broken chain ends where it breaks
        if (next == none() || !visited.insert(next).second || !records->contains(next)) {
            break;
        }

        Part part = decode_part(records->read(next));

        listing.last_id         = part.id;
        listing.last_is_part    = true;

        data_id = part.data_id;
        next    = part.next;
    }

    return listing;
}

bool Writer::find(const Header &directory, const std::string &stored, Found &found) const {
    std::string key = folded(stored);

    for (const auto &entry : list(directory).entries) {
        Header header;

        // An entry that points nowhere, or at no header, names nothing
        try {
            header = read_header(entry.id);
        } catch (const FULLFLASH::BaseException &) {
            continue;
        }

        if (header.id == entry.id && folded(header.name) == key) {
            found = { header, entry };

            return true;
        }
    }

    return false;
}

std::string Writer::new_name(const std::string &name) const {
    static const std::string forbidden = "\\/:*?\"<>|";

    if (name.empty() || name == "." || name == "..") {
        throw Exception("Invalid name '{}'", name);
    }

    for (char c : name) {
        if (static_cast<unsigned char>(c) < 0x20 || forbidden.find(c) != std::string::npos) {
            throw Exception("Invalid name '{}': no control characters and none of {}", name, forbidden);
        }
    }

    std::string stored = to_stored(name);

    check_new_name(stored);

    return stored;
}

uint32_t Writer::allocate() {
    return records->allocate_pair(first_id());
}

RawData Writer::empty_directory_record() const {
    return to_raw_data(std::string(directory_record_size(), static_cast<char>(0xFF)));
}

void Writer::add_entry(const Header &directory, uint32_t id, const std::string &stored) {
    Listing                 listing = list(directory);
    std::vector<uint8_t>    entry   = encode_entry(id, stored);

    if (listing.has_free) {
        records->patch(listing.free.record, listing.free.offset, entry);

        return;
    }

    // The directory is full: it goes on in a new part after its last
    Part part;

    part.id         = allocate();
    part.data_id    = part.id + 1;
    part.prev       = listing.last_id;
    part.next       = none();

    RawData record = empty_directory_record();

    record.write(0, reinterpret_cast<const char *>(entry.data()), entry.size());

    records->add(part.data_id, record);
    records->add(part.id, encode_part(part, directory));
    records->patch(listing.last_id, listing.last_is_part ? part_next_offset() : header_next_offset(), encode_id(part.id));
}

void Writer::delete_entry(const Entry &entry) {
    records->patch(entry.record, entry.offset, deleted_entry(records->read(entry.record), entry.offset));
}

void Writer::delete_records(const Header &header) {
    std::set<uint32_t>  visited;
    uint32_t            next = header.next_part;

    records->remove(header.id);

    if (records->contains(header.data_id)) {
        records->remove(header.data_id);
    }

    while (next != none() && visited.insert(next).second && records->contains(next)) {
        Part part = decode_part(records->read(next));

        records->remove(next);

        if (records->contains(part.data_id)) {
            records->remove(part.data_id);
        }

        next = part.next;
    }
}

void Writer::write_file(const std::vector<std::string> &path, const RawData &data, uint32_t fat_time) {
    Header      parent  = resolve_directory(std::vector<std::string>(path.begin(), path.end() - 1));
    std::string stored  = new_name(path.back());
    Found       existing;

    if (find(parent, stored, existing)) {
        if (existing.header.is_directory()) {
            throw Exception("{} is a directory", join(path));
        }

        delete_records(existing.header);
        delete_entry(existing.entry);
    }

    size_t size     = data.get_size();
    size_t pieces   = (size + chunk_size - 1) / chunk_size;

    Header header;

    header.id           = allocate();
    header.parent_id    = parent.id;
    header.data_id      = header.id + 1;
    header.size         = size;
    header.fat_time     = fat_time;
    header.attributes   = file_attributes();
    header.name         = stored;

    std::vector<uint32_t> part_ids;

    for (size_t i = 1; i < pieces; ++i) {
        part_ids.push_back(allocate());
    }

    header.next_part = part_ids.empty() ? none() : part_ids.front();

    for (size_t i = 0; i < pieces; ++i) {
        RawData piece(data, i * chunk_size, std::min<size_t>(chunk_size, size - i * chunk_size));

        if (i == 0) {
            records->add(header.data_id, piece);

            continue;
        }

        Part part;

        part.id         = part_ids[i - 1];
        part.data_id    = part.id + 1;
        part.prev       = i == 1 ? header.id : part_ids[i - 2];
        part.next       = i < part_ids.size() ? part_ids[i] : none();

        records->add(part.data_id, piece);
        records->add(part.id, encode_part(part, header));
    }

    records->add(header.id, encode_header(header));

    add_entry(parent, header.id, stored);
}

void Writer::create_directory(const std::vector<std::string> &path, uint32_t fat_time) {
    Header      parent  = resolve_directory(std::vector<std::string>(path.begin(), path.end() - 1));
    std::string stored  = new_name(path.back());
    Found       existing;

    if (find(parent, stored, existing)) {
        throw Exception("{} exists already", join(path));
    }

    Header header;

    header.id           = allocate();
    header.parent_id    = parent.id;
    header.data_id      = header.id + 1;
    header.next_part    = none();
    header.fat_time     = fat_time;
    header.attributes   = directory_attributes();
    header.name         = stored;

    records->add(header.data_id, empty_directory_record());
    records->add(header.id, encode_header(header));

    add_entry(parent, header.id, stored);
}

void Writer::remove(const std::vector<std::string> &path) {
    Header  parent = resolve_directory(std::vector<std::string>(path.begin(), path.end() - 1));
    Found   found;

    if (!find(parent, to_stored(path.back()), found)) {
        throw Exception("{} not found", join(path));
    }

    if (found.header.is_directory() && !list(found.header).entries.empty()) {
        throw Exception("Directory {} is not empty", join(path));
    }

    delete_records(found.header);
    delete_entry(found.entry);
}

// =========================================================================

// SGOLD2 and SGOLD2_ELKA: 32-bit ids, UTF-16 names, a file's data under the id after its header's
//
// Header: id, 0xFFFFFFFF, next part, parent id, size, FAT time (32 bits each), attributes, the
//         name's length (16 bits each), the name
// Part:   id, previous part or header, next part (32 bits each)
// Directory entry: id, 0xFFFF0000 | name hash (32 bits each), in records of 256 bytes
class NewSgoldWriter : public Writer {
    public:
        NewSgoldWriter(Records::Ptr records) : Writer(std::move(records)) {
            init();
        }

    protected:
        static constexpr size_t HEADER_SIZE     = 28;
        // Keeps a header inline in an ELKA FIT, where it can be changed in place
        static constexpr size_t NAME_LENGTH_MAX = (0x200 - HEADER_SIZE) / 2;

        uint32_t root_id() const override final {
            return 10;
        }

        uint32_t first_id() const override final {
            return 12;
        }

        uint32_t none() const override final {
            return 0xFFFFFFFF;
        }

        uint32_t read_chunk_size(const RawData &config) const override final {
            return read_value<uint32_t>(config, 4);
        }

        Header decode_header(const RawData &data) const override final {
            Header header;

            if (data.get_size() < HEADER_SIZE) {
                throw Exception("A header of {} bytes", data.get_size());
            }

            header.id           = read_value<uint32_t>(data, 0);
            header.next_part    = read_value<uint32_t>(data, 8);
            header.parent_id    = read_value<uint32_t>(data, 12);
            header.size         = read_value<uint32_t>(data, 16);
            header.fat_time     = read_value<uint32_t>(data, 20);
            header.attributes   = read_value<uint16_t>(data, 24);
            header.data_id      = header.id + 1;

            size_t name_size = std::min<size_t>(read_value<uint16_t>(data, 26) * 2, data.get_size() - HEADER_SIZE);

            if (name_size) {
                header.name.resize(name_size);
                data.read(HEADER_SIZE, &header.name[0], name_size);
            }

            return header;
        }

        RawData encode_header(const Header &header) const override final {
            std::string data(HEADER_SIZE, '\0');

            write_value<uint32_t>(data, 0,  header.id);
            write_value<uint32_t>(data, 4,  0xFFFFFFFF);
            write_value<uint32_t>(data, 8,  header.next_part);
            write_value<uint32_t>(data, 12, header.parent_id);
            write_value<uint32_t>(data, 16, header.size);
            write_value<uint32_t>(data, 20, header.fat_time);
            write_value<uint16_t>(data, 24, header.attributes);
            write_value<uint16_t>(data, 26, header.name.size() / 2);

            return to_raw_data(data + header.name);
        }

        Part decode_part(const RawData &data) const override final {
            Part part;

            part.id         = read_value<uint32_t>(data, 0);
            part.prev       = read_value<uint32_t>(data, 4);
            part.next       = read_value<uint32_t>(data, 8);
            part.data_id    = part.id + 1;

            return part;
        }

        RawData encode_part(const Part &part, const Header &owner) const override final {
            std::string data(12, '\0');

            write_value<uint32_t>(data, 0, part.id);
            write_value<uint32_t>(data, 4, part.prev);
            write_value<uint32_t>(data, 8, part.next);

            return to_raw_data(data);
        }

        size_t header_next_offset() const override final {
            return 8;
        }

        size_t part_next_offset() const override final {
            return 8;
        }

        std::vector<uint8_t> encode_id(uint32_t id) const override final {
            return value_bytes<uint32_t>(id);
        }

        uint32_t file_attributes() const override final {
            return 0x0000;
        }

        uint32_t directory_attributes() const override final {
            return 0x0010;
        }

        size_t directory_record_size() const override final {
            return 256;
        }

        size_t entry_size() const override final {
            return 8;
        }

        uint32_t entry_id(const RawData &record, size_t offset) const override final {
            return read_value<uint32_t>(record, offset);
        }

        std::vector<uint8_t> encode_entry(uint32_t id, const std::string &name) const override final {
            auto bytes  = value_bytes<uint32_t>(id);
            auto hash   = value_bytes<uint32_t>(0xFFFF0000 | name_hash_utf16(to_utf16(name)));

            bytes.insert(bytes.end(), hash.begin(), hash.end());

            return bytes;
        }

        std::vector<uint8_t> deleted_entry(const RawData &record, size_t offset) const override final {
            auto bytes  = value_bytes<uint32_t>(0);
            auto hash   = value_bytes<uint32_t>(read_value<uint32_t>(record, offset + 4) & 0xFFFF0000);

            bytes.insert(bytes.end(), hash.begin(), hash.end());

            return bytes;
        }

        std::string to_stored(const std::string &name) const override final {
            std::string stored;

            for (size_t i = 0; i < name.size();) {
                uint8_t     lead    = name[i];
                uint32_t    c;
                size_t      length;

                if (lead < 0x80) {
                    c = lead; length = 1;
                } else if ((lead & 0xE0) == 0xC0) {
                    c = lead & 0x1F; length = 2;
                } else if ((lead & 0xF0) == 0xE0) {
                    c = lead & 0x0F; length = 3;
                } else if ((lead & 0xF8) == 0xF0) {
                    c = lead & 0x07; length = 4;
                } else {
                    throw Exception("'{}' is not UTF-8", name);
                }

                if (i + length > name.size()) {
                    throw Exception("'{}' is not UTF-8", name);
                }

                for (size_t j = 1; j < length; ++j) {
                    uint8_t continuation = name[i + j];

                    if ((continuation & 0xC0) != 0x80) {
                        throw Exception("'{}' is not UTF-8", name);
                    }

                    c = (c << 6) | (continuation & 0x3F);
                }

                if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) {
                    throw Exception("'{}' is not UTF-8", name);
                }

                if (c >= 0x10000) {
                    c -= 0x10000;

                    append_unit(stored, 0xD800 | (c >> 10));
                    append_unit(stored, 0xDC00 | (c & 0x3FF));
                } else {
                    append_unit(stored, c);
                }

                i += length;
            }

            return stored;
        }

        void check_new_name(const std::string &stored) const override final {
            if (stored.size() / 2 > NAME_LENGTH_MAX) {
                throw Exception("Names are up to {} UTF-16 characters long", NAME_LENGTH_MAX);
            }
        }

        std::string folded(const std::string &stored) const override final {
            std::u16string name = to_utf16(stored);

            for (auto &c : name) {
                c = fold_case_utf16(c);
            }

            return std::string(reinterpret_cast<const char *>(name.data()), name.size() * 2);
        }

    private:
        static void append_unit(std::string &stored, uint32_t unit) {
            stored.push_back(static_cast<char>(unit & 0xFF));
            stored.push_back(static_cast<char>(unit >> 8));
        }

        static std::u16string to_utf16(const std::string &stored) {
            std::u16string name(stored.size() / 2, u'\0');

            if (!name.empty()) {
                memcpy(&name[0], stored.data(), name.size() * 2);
            }

            return name;
        }
};

// SGOLD: 16-bit ids, 8-bit names in the phone's codepage, or 0x1F and UTF-8
//
// Header: id, parent id (16 bits), FAT time (32), data id (16), attributes with the upper 16 bits
//         set (32), next part (16), the name ending in a 0
// Part:   id, the owner's parent id (16), the owner's FAT time (32), data id, the owner's
//         attributes, previous part or header, next part (16 each)
// Directory entry: id, name hash (16 bits each), in records of 128 bytes
class SgoldWriter : public Writer {
    public:
        SgoldWriter(Records::Ptr records, const std::string &codepage) : Writer(std::move(records)), codepage(codepage) {
            init();
        }

    protected:
        static constexpr size_t HEADER_SIZE     = 16;
        static constexpr size_t NAME_SIZE_MAX   = 255;

        std::string codepage;

        uint32_t root_id() const override final {
            return 6;
        }

        uint32_t first_id() const override final {
            return 10;
        }

        uint32_t none() const override final {
            return 0xFFFF;
        }

        uint32_t read_chunk_size(const RawData &config) const override final {
            return read_value<uint16_t>(config, 2);
        }

        Header decode_header(const RawData &data) const override final {
            Header header;

            if (data.get_size() < HEADER_SIZE) {
                throw Exception("A header of {} bytes", data.get_size());
            }

            header.id           = read_value<uint16_t>(data, 0);
            header.parent_id    = read_value<uint16_t>(data, 2);
            header.fat_time     = read_value<uint32_t>(data, 4);
            header.data_id      = read_value<uint16_t>(data, 8);
            header.attributes   = read_value<uint32_t>(data, 10);
            header.next_part    = read_value<uint16_t>(data, 14);

            data.read_string(HEADER_SIZE, header.name);

            return header;
        }

        RawData encode_header(const Header &header) const override final {
            std::string data(HEADER_SIZE, '\0');

            write_value<uint16_t>(data, 0,  header.id);
            write_value<uint16_t>(data, 2,  header.parent_id);
            write_value<uint32_t>(data, 4,  header.fat_time);
            write_value<uint16_t>(data, 8,  header.data_id);
            write_value<uint32_t>(data, 10, header.attributes);
            write_value<uint16_t>(data, 14, header.next_part);

            return to_raw_data(data + header.name + '\0');
        }

        Part decode_part(const RawData &data) const override final {
            Part part;

            part.id         = read_value<uint16_t>(data, 0);
            part.data_id    = read_value<uint16_t>(data, 8);
            part.prev       = read_value<uint16_t>(data, 12);
            part.next       = read_value<uint16_t>(data, 14);

            return part;
        }

        RawData encode_part(const Part &part, const Header &owner) const override final {
            std::string data(16, '\0');

            write_value<uint16_t>(data, 0,  part.id);
            write_value<uint16_t>(data, 2,  owner.parent_id);
            write_value<uint32_t>(data, 4,  owner.fat_time);
            write_value<uint16_t>(data, 8,  part.data_id);
            write_value<uint16_t>(data, 10, owner.attributes & 0xFFFF);
            write_value<uint16_t>(data, 12, part.prev);
            write_value<uint16_t>(data, 14, part.next);

            return to_raw_data(data);
        }

        size_t header_next_offset() const override final {
            return 14;
        }

        size_t part_next_offset() const override final {
            return 14;
        }

        std::vector<uint8_t> encode_id(uint32_t id) const override final {
            return value_bytes<uint16_t>(id);
        }

        uint32_t file_attributes() const override final {
            return 0xFFFF0000;
        }

        uint32_t directory_attributes() const override final {
            return 0xFFFF0010;
        }

        size_t directory_record_size() const override final {
            return 128;
        }

        size_t entry_size() const override final {
            return 4;
        }

        uint32_t entry_id(const RawData &record, size_t offset) const override final {
            return read_value<uint16_t>(record, offset);
        }

        std::vector<uint8_t> encode_entry(uint32_t id, const std::string &name) const override final {
            auto bytes  = value_bytes<uint16_t>(id);
            auto hash   = value_bytes<uint16_t>(name_hash_8bit(name));

            bytes.insert(bytes.end(), hash.begin(), hash.end());

            return bytes;
        }

        std::vector<uint8_t> deleted_entry(const RawData &record, size_t offset) const override final {
            return std::vector<uint8_t>(4, 0);
        }

        std::string to_stored(const std::string &name) const override final {
            return sgold_name_from_utf8(name, codepage);
        }

        void check_new_name(const std::string &stored) const override final {
            if (stored.size() > NAME_SIZE_MAX) {
                throw Exception("Names are up to {} bytes long", NAME_SIZE_MAX);
            }
        }

        // The firmware folds ASCII letters only: "Ärger" and "ärger" are two names to it
        std::string folded(const std::string &stored) const override final {
            std::string name = stored;

            for (auto &c : name) {
                c = fold_case_8bit(c);
            }

            return name;
        }
};

Writer::Ptr Writer::build(Platform::Type platform, Partitions::Partitions::Ptr partitions, const std::string &partition_name, const std::string &codepage) {
    Records::Ptr records = Records::build(platform, partitions, partition_name);

    switch (platform) {
        case Platform::Type::SGOLD:         return std::make_unique<SgoldWriter>(std::move(records), codepage);
        case Platform::Type::SGOLD2:
        case Platform::Type::SGOLD2_ELKA:   return std::make_unique<NewSgoldWriter>(std::move(records));
        default: {
            throw Exception("Writing is not supported on this platform");
        }
    }
}

};
};
};
