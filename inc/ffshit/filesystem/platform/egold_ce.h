#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_EGOLD_CARDEXPLORER_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_EGOLD_CARDEXPLORER_H

#include "ffshit/rawdata.h"

#include "ffshit/filesystem/platform/base.h"
#include "ffshit/filesystem/structure/structure.h"
#include "ffshit/partition/partitions.h"

#include <filesystem>

namespace FULLFLASH {
namespace Filesystem {

class EGOLD_CE : public Base {
    public:
        EGOLD_CE(Partitions::Partitions::Ptr partitions);

        static Base::Ptr build(Partitions::Partitions::Ptr partitions) {
            return std::make_shared<EGOLD_CE>(partitions);
        }

        void                        load(bool skip_broken = false, bool skip_dup = false, bool dump_data = false) override final;
        const Directory::Ptr        get_root() const override final;

    private:
        struct FITHeader {
            FITHeader() = default;

            uint16_t marker1    = 0x0;
            uint16_t size       = 0x0;
            uint32_t offset     = 0x0;
            uint16_t block_id   = 0x0;
            uint16_t marker2    = 0x0;
        };

        struct FFSBlock {
            FFSBlock() = default;

            FITHeader   header;
            RawData     data;
            uint32_t    addr_start  = 0x0;
            uint32_t    addr        = 0x0;
        };

        struct FileHeader {
            FileHeader() = default;

            uint16_t    id              = 0x0;
            uint16_t    parent_id       = 0x0;
            uint32_t    fat_timestamp   = 0x0;
            uint16_t    data_id         = 0x0;
            uint16_t    flags           = 0x0;
            uint16_t    unk4            = 0x0;
            uint16_t    next_part_id    = 0x0;
            std::string name;
        };

        struct FFSFile {
            FFSFile() = default;

            FileHeader          header;
            const FFSBlock *    block;
        };

        using FFSBlocksMap = std::map<uint16_t, FFSBlock>;
        using FFSFilesMap = std::map<uint16_t, FFSFile>;

        static constexpr uint32_t   ID_ADD = 6000;

        Partitions::Partitions::Ptr partitions;
        Directory::Ptr              root_dir;

        std::vector<uint32_t>       recourse_protector;

        void                        parse_FIT(bool skip_broken, bool skip_dup, bool dump_data);

        void                        print_block_header(const FFSBlock &block);
        void                        print_file_header(const FFSFile &file);
        void                        print_data(const FFSBlock &block);

        void                        scan(const std::string &part_name, const FFSBlocksMap &ffs_blocks, const FFSFilesMap &ffs_files, const FFSFile &file, Directory::Ptr dir, bool skip_broken, std::string path = "/");
        void                        read_full(const FFSBlocksMap &ffs_map, const FFSFilesMap &ffs_files, const FFSFile &file, RawData &data);
        void                        read_recurse(const FFSBlocksMap &ffs_map, const FFSFilesMap &ffs_files, const FFSFile &file, RawData &data, uint16_t next_file_id);

    };

};
};

#endif
