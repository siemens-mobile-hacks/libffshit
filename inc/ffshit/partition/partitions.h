#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/platform/types.h"
#include "ffshit/partition/partition.h"
#include "ffshit/patterns/pattern.h"


#include <memory>
#include <map>
#include <regex>

namespace FULLFLASH {
namespace Partitions {

static constexpr size_t     BC65_BC75_OFFSET    = 0x870;
static constexpr size_t     BC85_OFFSET         = 0xC70;

static constexpr size_t     X65_MODEL_OFFSET    = 0x210;
static constexpr size_t     X75_MODEL_OFFSET    = 0x210;
static constexpr size_t     X85_MODEL_OFFSET    = 0x3E000;

static constexpr size_t     X65_IMEI_OFFSET     = 0x65C;
static constexpr size_t     X65_7X_IMEI_OFFSET  = 0x660;
static constexpr size_t     X75_IMEI_OFFSET     = 0x660;
static constexpr size_t     X85_IMEI_OFFSET     = 0x3E410;

static constexpr uint32_t   FF_ADDRESS_MASK     = 0x0FFFFFFF;

static constexpr uint32_t   X65_SEARCH_START_ADDR   =   0x00800000;
static constexpr uint32_t   X75_SEARCH_START_ADDR   =   0x004C0000;

class Partitions {
    public:
        using Ptr       = std::shared_ptr<Partitions>;
        using Map       = std::map<std::string, Partition>;
        
        template<typename ...Args>
        static Ptr build(Args ...args) {
            return std::make_unique<Partitions>(args...);
        }

        Partitions(std::string fullflash_path, bool old_search_alghoritm, bool search_from_addr = false, uint32_t search_start_addr = 0);
        Partitions(std::string fullflash_path, Platform platform, bool old_search_alghoritm, bool search_from_addr = false, uint32_t search_start_addr = 0);

        const std::string &         get_file_path() const;

        const Map &                 get_partitions() const;
        const RawData &             get_data() const;

        const Platform              get_platform() const;
        const std::string &         get_imei() const;
        const std::string &         get_model() const;

    private:
        std::string                 fullflash_path;

        uint32_t                    block_size;
        Map                         partitions_map;
        
        RawData                     data;
        Platform                    platform;
        std::string                 imei;
        std::string                 model;

        bool                        sl75_bober_kurwa;

        bool                        search_partitions_x65(uint32_t start_addr);
        bool                        search_partitions_x75(uint32_t start_addr);
        bool                        search_partitions_x85(uint32_t start_addr);

        void                        detect_platform();
        void                        old_search_blocks();
        void                        old_search_blocks_x85();

        std::vector<uint32_t>       find_pattern(const Patterns::Readable &pattern_readable, uint32_t start = 0, bool break_first = false);


};


};
};

#endif
