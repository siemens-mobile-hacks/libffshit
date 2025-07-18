#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_SGOLD_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_SGOLD_H

#include "ffshit/rawdata.h"

#include "ffshit/filesystem/platform/base.h"
#include "ffshit/filesystem/structure/structure.h"
#include "ffshit/partition/partitions.h"

namespace FULLFLASH {
namespace Filesystem {

class SGOLD : public Base {
    public:
        SGOLD(Partitions::Partitions::Ptr partitions);

        static Base::Ptr build(Partitions::Partitions::Ptr partitions) {
            return std::make_shared<SGOLD>(partitions);
        }

        void                    load(bool skip_broken = false, bool skip_dup = false, bool dump_data = false) override final;
        const Directory::Ptr    get_root() const override final;

    private:
        struct FITHeader {
            FITHeader() = default;

            uint32_t flags      = 0x0;
            uint32_t id         = 0x0;
            uint32_t size       = 0x0;
            uint32_t offset     = 0x0;
        };

        struct FFSBlock {
            FFSBlock() = default;

            FITHeader   header;
            RawData     data;
        };

        struct FileHeader {
            FileHeader() = default;

            uint16_t    id              = 0x0;
            uint16_t    parent_id       = 0x0;
            uint32_t    fat_timestamp   = 0x0;
            uint16_t    data_id         = 0x0;
            uint32_t    attributes      = 0x0;
            uint16_t    next_part       = 0x0;
            std::string name;
        };

        struct FilePart {
            FilePart() = default;

            uint16_t    id              = 0x0;
            uint16_t    parent_id       = 0x0;
            uint32_t    unknown         = 0x0;
            uint16_t    data_id         = 0x0;
            uint16_t    unknown2        = 0x0;
            uint16_t    prev_id         = 0x0;
            uint16_t    next_part       = 0x0;
        };

        using FSBlocksMap   = std::map<uint16_t, FFSBlock>;

        Partitions::Partitions::Ptr partitions;
        Directory::Ptr              root_dir;

        std::vector<uint32_t>       recourse_protector;

        bool                        prototype_6000;

        void                        parse_FIT(bool skip_broken, bool skip_dup, bool dump_data);

        void                        scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, bool skip_broken = false, std::string path = "/");
        void                        read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id);
        RawData                     read_full_data(FSBlocksMap &ffs_map, const FileHeader &header);

        static void                 print_fit_header(const SGOLD::FITHeader &header);
        static void                 print_file_header(const SGOLD::FileHeader &header);
        static void                 print_file_part(const SGOLD::FilePart &part);
        static void                 print_data(const FFSBlock &block);

        static SGOLD::FileHeader    read_file_header(const RawData &data);
        static SGOLD::FilePart      read_file_part(const RawData &data);
};

};
};

#endif
