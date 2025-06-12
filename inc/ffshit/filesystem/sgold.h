#ifndef FULLFLASH_FILESYSTEM_SGOLD_H
#define FULLFLASH_FILESYSTEM_SGOLD_H

#include "ffshit/blocks.h"
#include "ffshit/rawdata.h"

#include "ffshit/filesystem/base.h"
#include "ffshit/filesystem/file.h"
#include "ffshit/filesystem/directory.h"

namespace FULLFLASH {
namespace Filesystem {

class SGOLD : public Base {
    public:
        SGOLD(Blocks &blocks);

        static Base::Ptr build(Blocks &blocks) {
            return std::make_shared<SGOLD>(blocks);
        }

        void    load() override final;
        void    extract(std::string path, bool overwrite) override final;

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
        using FSMap         = std::map<std::string, Directory::Ptr>;

        Blocks  &                   blocks;
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

        void                        unpack(Directory::Ptr dir, std::string path = "/");
};

};
};

#endif
