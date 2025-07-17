#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/platform/types.h"
#include "ffshit/partition/partition.h"
#include "ffshit/patterns/pattern.h"


#include <memory>
#include <map>
#include <regex>
#include <filesystem>

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

static constexpr size_t     EGOLD_INFO_OFFSET1          = 0x400300;
static constexpr size_t     EGOLD_INFO_OFFSET2          = 0x600300;
static constexpr size_t     EGOLD_INFO_OFFSET3          = 0x800300;

static std::vector<size_t>  EGOLD_INFO_OFFSETS = { EGOLD_INFO_OFFSET1, EGOLD_INFO_OFFSET2, EGOLD_INFO_OFFSET3 };

static constexpr size_t     EGOLD_MODEL_OFFSET          = 0x0C;
static constexpr size_t     EGOLD_MAGICK_SIEMENS_OFFSET = 0x1C;

class Partitions {
    public:
        using Ptr       = std::shared_ptr<Partitions>;
        using Map       = std::map<std::string, Partition>;
        
        template<typename ...Args>
        static Ptr build(Args ...args) {
            return std::make_unique<Partitions>(args...);
        }

        Partitions(std::filesystem::path fullflash_path, bool old_search_algorithm, uint32_t search_start_addr = 0);
        Partitions(std::filesystem::path fullflash_path, Platform from_platform, bool old_search_algorithm, uint32_t search_start_addr = 0);

        Partitions(char *ff_data, size_t ff_data_size, bool old_search_algorithm, uint32_t search_start_addr = 0);
        Partitions(char *ff_data, size_t ff_data_size, Platform from_platform, bool old_search_algorithm, uint32_t search_start_addr = 0);

        const std::filesystem::path &   get_file_path() const;

        const Map &                     get_partitions() const;
        const RawData &                 get_data() const;

        const Platform                  get_platform() const;
        const std::string &             get_imei() const;
        const std::string &             get_model() const;

    private:
        std::filesystem::path       fullflash_path;

        uint32_t                    block_size;
        Map                         partitions_map;
        
        RawData                     data;
        Platform                    platform;
        std::string                 imei;
        std::string                 model;

        bool                        sl75_bober_kurwa;

        void                        process(bool old_search_algorithm, uint32_t search_start_addr);
        void                        process(Platform from_platform, bool old_search_algorithm, uint32_t search_start_addr);

        void                        x65flasher_fix();

        bool                        check_part_name(const std::string &name);

        void                        search_partitions(bool old_search_algorithm, uint32_t start_addr);

        bool                        search_partitions_sgold(uint32_t start_addr);
        bool                        search_partitions_sgold2(uint32_t start_addr);
        bool                        search_partitions_sgold2_elka(uint32_t start_addr);

        void                        detect_platform();

        void                        old_search_partitions_egold_ce();
        void                        old_search_partitions_sgold_sgold2();
        void                        old_search_partitions_sgold2_elka();

        std::vector<uint32_t>       find_pattern(const Patterns::Readable &pattern_readable, uint32_t start = 0, bool break_first = false);


};


};
};

#endif
