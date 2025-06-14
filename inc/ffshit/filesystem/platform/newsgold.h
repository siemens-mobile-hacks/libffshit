#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_NEWSGOLD_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_NEWSGOLD_H

#include "ffshit/blocks.h"

#include "ffshit/filesystem/platform/base.h"
#include "ffshit/filesystem/structure/structure.h"

namespace FULLFLASH {
namespace Filesystem {

class NewSGOLD : public Base {
    public:
        NewSGOLD(Blocks &blocks);

        static Base::Ptr build(Blocks &blocks) {
            return std::make_shared<NewSGOLD>(blocks);
        }

        void                        load() override final;
        // void                        extract(std::string path, bool overwrite) override final;
        const FSMap &               get_filesystem_map() const override final;

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
            uint16_t    unknown4;
            uint16_t    unknown5;
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

        Blocks &                    blocks;
        FSMap                       fs_map;

        static void                 print_fit_header(const NewSGOLD::FITHeader &header);
        void                        parse_FIT();

        static void                 print_file_header(const FileHeader &header);
        static void                 print_file_part(const FilePart &part);

        static FileHeader           read_file_header(const RawData &data);
        static FilePart             read_file_part(const RawData &data);

        void                        read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id);
        RawData                     read_full_data(FSBlocksMap &ffs_map, const FileHeader &header);
        void                        scan(const std::string &block_name, FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, std::string path = "/");

        void                        unpack(Directory::Ptr dir, std::string path = "/");

};

};
};


#endif
