#ifndef BLOCKS_H
#define BLOCKS_H

#define FMT_HEADER_ONLY

#include <fmt/format.h>
#include <fmt/printf.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

#include "rawdata.h"

namespace FULLFLASH {

enum class Platform {
    UNK,
    X65,
    X75,
    X85
};

const std::unordered_map<Platform, std::string> PlatformToString = {
    { Platform::X65, "X65" },
    { Platform::X75, "X75" },
    { Platform::X85, "X85" },
};

const std::unordered_map<std::string, Platform> StringToPlatform = {
    { "X65", Platform::X65 },
    { "X75", Platform::X75 },
    { "X85", Platform::X85 },
};

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
