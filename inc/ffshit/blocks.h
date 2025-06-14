#ifndef LIBFFSHIT_BLOCKS_H
#define LIBFFSHIT_BLOCKS_H

#include <fmt/format.h>
#include <fmt/printf.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/platform/types.h"

namespace FULLFLASH {

class Blocks {
    public:
        typedef struct {
            char        name[8];
            uint16_t    unknown_1;
            uint16_t    unknown_2;
            uint32_t    unknown_3;
        } BlockHeader;

        typedef struct {
            BlockHeader             header;
            size_t                  offset;
            size_t                  count;
            RawData                 data;
            std::string             name;
        } Block;

        using Map   = std::map<std::string, std::vector<Block>>;
        using Ptr   = std::unique_ptr<Blocks>;

        template<typename ...Args>
        static Ptr build(Args ...args) {
            return std::make_unique<Blocks>(args...);
        }

        Blocks(std::string fullflash_path);
        Blocks(std::string fullflash_path, Platform platform);

        const Platform              get_platform() const;
        const std::string &         get_imei() const;
        const std::string &         get_model() const;

        Map &                       get_blocks();
        RawData &                   get_data();

        void                        print();
        uint32_t                    block_size;

        // static const uint32_t       block_size = 0x10000;

    private:
        void                        detect_platform();
        void                        search_blocks();
        void                        search_blocks_x85();

        RawData                     data;
        Map                         blocks_map;
        Platform                    platform;
        std::string                 imei;
        std::string                 model;

};
};

#endif
