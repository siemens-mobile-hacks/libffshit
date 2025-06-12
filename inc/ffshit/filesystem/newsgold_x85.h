#ifndef FULLFLASH_FILESYSTEM_NEWSGOLD_X85_H
#define FULLFLASH_FILESYSTEM_NEWSGOLD_X85_H

#include "ffshit/blocks.h"
#include "ffshit/filesystem/base.h"
#include "ffshit/filesystem/file.h"
#include "ffshit/filesystem/directory.h"

namespace FULLFLASH {
namespace Filesystem {

class NewSGOLD_X85 : public Base {
    public:
        NewSGOLD_X85(Blocks &blocks);

        static Base::Ptr build(Blocks &blocks) {
            return std::make_shared<NewSGOLD_X85>(blocks);
        }

        void                        load() override final;
        void                        extract(std::string path, bool overwrite) override final;

    private:
        typedef struct {
            uint32_t    flags;
            uint32_t    id;
            uint32_t    size;
            uint32_t    offset;
            uint32_t    unk1;
            uint32_t    unk2;
            uint32_t    unk3;
            uint32_t    unk4;
        } FITHeader;

        typedef struct {
            FITHeader               header;
            RawData                 data;
            RawData                 data_header;
            RawData                 data_from_header;
            const Blocks::Block *   block_ptr;
            uint32_t                ff_boffset;
            uint32_t                ff_offset;
            uint16_t                fit_size;
        } FFSBlock;

        typedef struct {
            uint32_t    id;
            uint32_t    unknown1;
            uint32_t    next_part;
            uint32_t    parent_id;
            uint32_t    unknown2;
            uint16_t    unknown4;
            uint16_t    unknown5;
            uint16_t    unknown6;
            uint16_t    unknown7;
            std::string name;
        } FileHeader;

        typedef struct {
            uint16_t    id;
            uint16_t    unknown1;
            uint16_t    unknown2;
            uint16_t    unknown3;
        } DirHeader;


        using DirList = std::vector<DirHeader>;

        typedef struct {
            uint32_t    id;
            uint32_t    parent_id;
            uint32_t    next_part;
        } FilePart;

        using FSBlocksMap       = std::map<uint16_t, FFSBlock>;
        using FSBlocksMapList   = std::map<uint16_t, std::vector<FFSBlock>>;

        using FSMap             = std::map<std::string, Directory::Ptr>;

        Blocks &                    blocks;
        FSMap                       fs_map;

        static void                 dump_data(const RawData &raw_data);
        static void                 dump_block(const NewSGOLD_X85::FFSBlock &block, bool is_dump_data = true);

        static void                 print_dir_header(const DirHeader &header);
        static void                 print_file_header(const FileHeader &header);
        static void                 print_file_part(const FilePart &part);

        static RawData              read_aligned(const FFSBlock &block);
        static RawData              read_aligned(char *data, size_t size);
        static FileHeader           read_file_header(const FFSBlock &block);
        // static FileHeader           read_file_header2(const FFSBlock &block);
        static FilePart             read_file_part(const FFSBlock &block);

        uint32_t                    read_part(const FFSBlock &prev_part, const FileHeader &file_header, const FSBlocksMap &ffs_map_C0, const FSBlocksMapList &ffs_map_00, uint32_t next_part, uint32_t last_offset, RawData &data, size_t start_offset);
        void                        read_file(const FSBlocksMap &ffs_map_C0, const FSBlocksMapList &ffs_map_00, const FFSBlock &file_block, RawData &data);

        static void                 print_fit_header(const NewSGOLD_X85::FITHeader &header);
        void                        parse_FIT();

        DirList                     get_directory(FSBlocksMap &ffs_map_C0, FSBlocksMapList &ffs_map_00, const FFSBlock &block);
        void                        scan(FSBlocksMap &ffs_map_C0, FSBlocksMapList &ffs_map_00, Directory::Ptr dir, const FFSBlock &block, std::string path = "/");

        void                        unpack(Directory::Ptr dir, std::string path = "/");

};

};
};

#endif
