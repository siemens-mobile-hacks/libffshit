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

        void                        load(bool skip_broken = false, bool skip_dup = false) override final;
        const Directory::Ptr        get_root() const override final;

    private:
        typedef struct {
            uint32_t    flags;
            uint32_t    id;
            uint32_t    size;
            uint32_t    offset;
        } FITHeader;

        typedef struct {
            FITHeader   header;
            RawData     data;
        } FFSBlock;

        typedef struct {
            uint32_t    id;
            uint32_t    unknown1;
            uint32_t    next_part;
            uint32_t    parent_id;
            uint16_t    unknown2;
            uint16_t    unknown3;
            uint32_t    fat_timestamp;
            uint16_t    unknown6;
            uint16_t    unknown7;
            std::string name;
        } FileHeader;

        typedef struct {
            uint32_t    id;
            uint32_t    parent_id;
            uint32_t    next_part;
        } FilePart;

        using FSBlocksMap   = std::map<uint16_t, FFSBlock>;

        Partitions::Partitions::Ptr partitions;
        Directory::Ptr              root_dir;

        std::vector<uint32_t>       recourse_protector;

        static void                 print_fit_header(const SGOLD2::FITHeader &header);
        void                        parse_FIT(bool skip_broken, bool skip_dup);

        static void                 print_file_header(const FileHeader &header);
        static void                 print_file_part(const FilePart &part);

        static FileHeader           read_file_header(const RawData &data);
        static FilePart             read_file_part(const RawData &data);

        void                        read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id);
        RawData                     read_full_data(FSBlocksMap &ffs_map, const FileHeader &header);
        void                        scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, bool skip_broken = false, std::string path = "/");

};

};
};


#endif
