#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_EGOLD_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_EGOLD_H

#include "ffshit/rawdata.h"

#include "ffshit/filesystem/platform/base.h"
#include "ffshit/filesystem/structure/structure.h"
#include "ffshit/partition/partitions.h"

#include <filesystem>

namespace FULLFLASH {
namespace Filesystem {

class EGOLD : public Base {
    public:
        EGOLD(Partitions::Partitions::Ptr partitions);

        static Base::Ptr build(Partitions::Partitions::Ptr partitions) {
            return std::make_shared<EGOLD>(partitions);
        }

        void                        load() override final;
        const FSMap &               get_filesystem_map() const override final;

    private:
        typedef struct {
            uint16_t marker1;
            uint16_t size;
            uint32_t offset;
            // uint16_t unk1;
            uint16_t block_id;
            uint16_t marker2;
        } FITHeader;

        typedef struct {
            FITHeader   header;
            RawData     data;
            uint32_t    addr_start;
            uint32_t    addr;
        } FFSBlock;

        typedef struct {
            uint16_t    id;
            uint16_t    parent_id;
            uint32_t    fat_timestamp;
            uint16_t    data_id;
            uint16_t    flags;
            uint16_t    unk4;
            uint16_t    next_part_id;
            std::string name;
        } FileHeader;

        typedef struct {
            FileHeader          header;
            const FFSBlock *    block;
        } FFSFile;

        Partitions::Partitions::Ptr partitions;
        FSMap                       fs_map;
        // std::map<uint16_t, FFSBlock> ffs_blocks;
        // std::map<uint16_t, FFSFile>  ffs_files;

        using FFSBlocksMap = std::map<uint16_t, FFSBlock>;
        using FFSFilesMap = std::map<uint16_t, FFSFile>;

        void                        parse_FIT();

        void                        print_block_header(const FFSBlock &block);
        void                        print_file_header(const FFSFile &file);
        void                        print_data(const FFSBlock &block);

        void                        scan(const FFSBlocksMap &ffs_blocks, const FFSFilesMap &ffs_files, const FFSFile &file, Directory::Ptr dir, std::filesystem::path path);
        void                        read_full(const FFSBlocksMap &ffs_map, const FFSFilesMap &ffs_files, const FFSFile &file, RawData &data);
        void                        read_recurse(const FFSBlocksMap &ffs_map, const FFSFilesMap &ffs_files, const FFSFile &file, RawData &data, uint16_t next_file_id);

    };

};
};

#endif
