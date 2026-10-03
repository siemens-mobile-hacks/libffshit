#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_WRITE_WRITER_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_WRITE_WRITER_H

#include "filesystem/write/records.h"

#include <string>
#include <vector>

namespace FULLFLASH {
namespace Filesystem {
namespace Write {

// A file's or directory's header
struct Header {
    uint32_t    id          = 0;
    uint32_t    parent_id   = 0;
    uint32_t    data_id     = 0;
    uint32_t    next_part   = 0;
    uint32_t    size        = 0;
    uint32_t    fat_time    = 0;
    uint32_t    attributes  = 0;
    // as the header keeps it: UTF-16LE, or 8-bit
    std::string name;

    bool is_directory() const {
        return attributes & 0x10;
    }
};

// Where the data goes on: the next piece of a file, or the next record of a directory's entries
struct Part {
    uint32_t    id          = 0;
    uint32_t    data_id     = 0;
    uint32_t    prev        = 0;
    uint32_t    next        = 0;
};

// Files and directories on the records of one partition. A file is a header, its data in pieces
// of the partition's chunk size, and a part for every piece after the first. A directory is a
// header and records of entries, each the id of a header and the hash of its name, which grow by
// parts too.
class Writer {
    public:
        using Ptr = std::unique_ptr<Writer>;

        // SGOLD keeps names in the codepage
        static Ptr              build(Platform::Type platform, Partitions::Partitions::Ptr partitions, const std::string &partition_name, const std::string &codepage);

        virtual                 ~Writer() = default;

        // The paths are below the partition's root directory, the names in UTF-8
        void                    write_file(const std::vector<std::string> &path, const RawData &data, uint32_t fat_time);
        void                    create_directory(const std::vector<std::string> &path, uint32_t fat_time);
        void                    remove(const std::vector<std::string> &path);

        // Whether the firmware takes the names for the same one
        bool                    same_name(const std::string &a, const std::string &b) const;

        Records &               get_records();

    protected:
        Records::Ptr            records;
        uint32_t                chunk_size;

        Writer(Records::Ptr records);

        // Checks the partition and reads its chunk size. Called by the formats once they are built.
        void                    init();

        // What differs between the formats
        virtual uint32_t        root_id() const = 0;
        // The first id that is neither the firmware's own record nor the root's
        virtual uint32_t        first_id() const = 0;
        // The id that is none: no next part, a free directory entry
        virtual uint32_t        none() const = 0;
        virtual uint32_t        read_chunk_size(const RawData &config) const = 0;

        virtual Header          decode_header(const RawData &data) const = 0;
        virtual RawData         encode_header(const Header &header) const = 0;
        virtual Part            decode_part(const RawData &data) const = 0;
        virtual RawData         encode_part(const Part &part, const Header &owner) const = 0;
        virtual size_t          header_next_offset() const = 0;
        virtual size_t          part_next_offset() const = 0;
        virtual std::vector<uint8_t> encode_id(uint32_t id) const = 0;

        virtual uint32_t        file_attributes() const = 0;
        virtual uint32_t        directory_attributes() const = 0;

        virtual size_t          directory_record_size() const = 0;
        virtual size_t          entry_size() const = 0;
        // none() for a free entry, 0 for a deleted one
        virtual uint32_t        entry_id(const RawData &record, size_t offset) const = 0;
        virtual std::vector<uint8_t> encode_entry(uint32_t id, const std::string &name) const = 0;
        virtual std::vector<uint8_t> deleted_entry(const RawData &record, size_t offset) const = 0;

        // A name in UTF-8 as a header keeps it
        virtual std::string     to_stored(const std::string &name) const = 0;
        // Throws when the firmware could not take the name
        virtual void            check_new_name(const std::string &stored) const = 0;
        // The same for two names the firmware takes for the same one
        virtual std::string     folded(const std::string &stored) const = 0;

    private:
        struct Entry {
            // the record of directory entries holding it, and where
            uint32_t    record;
            size_t      offset;
            uint32_t    id;
        };

        struct Listing {
            std::vector<Entry>  entries;
            bool                has_free    = false;
            Entry               free        = {};
            // the header, or the part, that ends the directory
            uint32_t            last_id     = 0;
            bool                last_is_part = false;
        };

        struct Found {
            Header      header;
            Entry       entry;
        };

        Header                  read_header(uint32_t id) const;
        Header                  resolve_directory(const std::vector<std::string> &path) const;
        Listing                 list(const Header &directory) const;
        bool                    find(const Header &directory, const std::string &stored, Found &found) const;

        std::string             new_name(const std::string &name) const;
        uint32_t                allocate();
        RawData                 empty_directory_record() const;
        void                    add_entry(const Header &directory, uint32_t id, const std::string &stored);
        void                    delete_entry(const Entry &entry);
        void                    delete_records(const Header &header);
};

};
};
};

#endif
