#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_SGOLD2_ELKA_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_SGOLD2_ELKA_H

#include "ffshit/filesystem/platform/base.h"
#include "ffshit/filesystem/structure/structure.h"
#include "ffshit/partition/partitions.h"

namespace FULLFLASH {
namespace Filesystem {

class SGOLD2_ELKA : public Base {
    public:
        SGOLD2_ELKA(Partitions::Partitions::Ptr partitions);

        static Base::Ptr build(Partitions::Partitions::Ptr partitions) {
            return std::make_shared<SGOLD2_ELKA>(partitions);
        }

        void                        load(bool skip_broken = false, bool skip_dup = false, bool dump_data = false) override final;
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

            FITHeader               header;
            RawData                 data;
        };

        struct FileHeader {
            FileHeader() = default;

            uint32_t    id              = 0x0;
            uint32_t    unknown1        = 0x0;
            uint32_t    next_part       = 0x0;
            uint32_t    parent_id       = 0x0;
            uint32_t    size            = 0x0;
            uint32_t    fat_timestamp   = 0x0;
            uint16_t    attributes      = 0x0;
            uint16_t    unknown7        = 0x0;
            std::string name;
        };

        struct DirHeader {
            DirHeader() = default;

            uint16_t    id              = 0x0;
            uint16_t    unknown1        = 0x0;
            uint16_t    unknown2        = 0x0;
            uint16_t    unknown3        = 0x0;
        };

        using DirList = std::vector<DirHeader>;

        struct FilePart {
            FilePart() = default;

            uint32_t    id              = 0x0;
            uint32_t    parent_id       = 0x0;
            uint32_t    next_part       = 0x0;
        };

        using FSBlocksMap       = std::map<uint16_t, FFSBlock>;
        using FSBlocksMapList   = std::map<uint16_t, std::vector<FFSBlock>>;

        Partitions::Partitions::Ptr partitions;
        Directory::Ptr              root_dir;

        std::vector<uint32_t>       recourse_protector;

        void                        parse_FIT(bool skip_broken, bool skip_dup, bool dump_data);

        static void                 print_fit_header(const SGOLD2_ELKA::FITHeader &header);
        static void                 print_dir_header(const DirHeader &header);
        static void                 print_file_header(const FileHeader &header);
        static void                 print_file_part(const FilePart &part);
        static void                 print_data(const FFSBlock &block);

        static FileHeader           read_file_header(const FFSBlock &block);
        static FilePart             read_file_part(const RawData &data);

        void                        scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, bool skip_broken = false, std::string path = "/");
        RawData                     read_full_data(FSBlocksMap &ffs_map, const FileHeader &header);
        void                        read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id);


};

};
};

#endif
