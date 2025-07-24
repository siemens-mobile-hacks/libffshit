#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_SGOLD2_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_SGOLD2_H

#include "ffshit/filesystem/platform/base.h"
#include "ffshit/filesystem/structure/structure.h"
#include "ffshit/partition/partitions.h"

namespace FULLFLASH {
namespace Filesystem {

class SGOLD2 : public Base {
    public:
        SGOLD2(Partitions::Partitions::Ptr partitions);

        static Base::Ptr build(Partitions::Partitions::Ptr partitions) {
            return std::make_shared<SGOLD2>(partitions);
        }

        void                        load(bool skip_broken = false, bool skip_dup = false, std::vector<std::string> parts_to_extract = {}) override final;
        const Directory::Ptr        get_root() const override final;

    private:
        struct FITHeader {
            FITHeader() = default;

            uint32_t    flags   = 0x0;
            uint32_t    id      = 0x0;
            uint32_t    size    = 0x0;
            uint32_t    offset  = 0x0;
        };

        struct FFSBlock {
            FFSBlock() = default;

            FITHeader   header;
            RawData     data;
        };

        struct FileHeader {
            FileHeader() = default;

            uint32_t    id              = 0x0;
            uint32_t    unknown1        = 0x0;
            uint32_t    next_part       = 0x0;
            uint32_t    parent_id       = 0x0;
            uint16_t    unknown2        = 0x0;
            uint16_t    unknown3        = 0x0;
            uint32_t    fat_timestamp   = 0x0;
            uint16_t    attributes      = 0x0;
            uint16_t    unknown7        = 0x0;
            std::string name;
        };

        struct FilePart {
            FilePart() = default;

            uint32_t    id              = 0x0;
            uint32_t    parent_id       = 0x0;
            uint32_t    next_part       = 0x0;
        };

        using FSBlocksMap   = tsl::ordered_map<uint16_t, FFSBlock>;

        Partitions::Partitions::Ptr partitions;
        Directory::Ptr              root_dir;

        std::vector<uint32_t>       recourse_protector;

        void                        print_fit_header(const SGOLD2::FITHeader &header);
        void                        print_file_header(const FileHeader &header);
        void                        print_file_part(const FilePart &part);
        void                        print_data(const FFSBlock &block);

        void                        parse_FIT(bool skip_broken, bool skip_dup, std::vector<std::string> parts_to_extract);

        static FileHeader           read_file_header(const RawData &data);
        static FilePart             read_file_part(const RawData &data);

        void                        read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id);
        void                        read_full_data(FSBlocksMap &ffs_map, const FileHeader &header, RawData &file_data);
        void                        scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, bool skip_broken = false, std::string path = "/");

};

};
};


#endif
