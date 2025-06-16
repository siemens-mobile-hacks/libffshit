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

        void            load() override final;
        const FSMap &   get_filesystem_map() const override final;

    private:
        typedef struct {
            uint32_t flags;
            uint32_t id;
            uint32_t size;
            uint32_t offset;
        } FITHeader;

        typedef struct {
            FITHeader   header;
            RawData     data;
        } FFSBlock;

        typedef struct {
            uint16_t    id;
            uint16_t    parent_id;
            uint32_t    unknown;
            uint16_t    data_id;
            uint32_t    attributes;
            uint16_t    next_part;
            std::string name;
        } FileHeader;

        typedef struct {
            uint16_t    id;
            uint16_t    parent_id;
            uint32_t    unknown;
            uint16_t    data_id;
            uint16_t    unknown2;
            uint16_t    prev_id;
            uint16_t    next_part;
        } FilePart;

        using FSBlocksMap   = std::map<uint16_t, FFSBlock>;

        Partitions::Partitions::Ptr partitions;
        FSMap                       fs_map;

        void                        parse_FIT();

        void                        scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, std::string path = "/");
        void                        read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id);
        RawData                     read_full_data(FSBlocksMap &ffs_map, const FileHeader &header);

        static void                 print_fit_header(const SGOLD::FITHeader &header);
        static void                 print_file_header(const SGOLD::FileHeader &header);
        static void                 print_file_part(const SGOLD::FilePart &part);

        static SGOLD::FileHeader    read_file_header(const RawData &data);
        static SGOLD::FilePart      read_file_part(const RawData &data);
};

};
};

#endif
