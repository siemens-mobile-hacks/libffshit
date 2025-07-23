#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/platform/types.h"
#include "ffshit/partition/partition.h"
#include "ffshit/patterns/pattern.h"

#include "ffshit/detector.h"

#include "thirdparty/ordered_map.h"

#include <memory>
#include <regex>
#include <filesystem>

namespace FULLFLASH {
namespace Partitions {

class Partitions {
    public:
        using Ptr       = std::shared_ptr<Partitions>;
        using Map       = tsl::ordered_map<std::string, Partition>;
        
        Partitions(const RawData& raw_data, Detector::Ptr detector, bool old_search_algorithm, uint32_t search_start_addr = 0);

        static Ptr build(const RawData& raw_data, Detector::Ptr detector, bool old_search_algorithm, uint32_t search_start_addr = 0) {
            return std::make_unique<Partitions>(raw_data, detector, old_search_algorithm, search_start_addr);
        }

        const Map &                 get_partitions() const;
        const RawData &             get_data() const;
        const Detector::Ptr &       get_detector() const;

    private:
        uint32_t                    block_size;
        Map                         partitions_map;
        
        Detector::Ptr               detector;
        const RawData &             data;

        bool                        check_part_name(const std::string &name);

        void                        search_partitions(bool old_search_algorithm, uint32_t start_addr);

        bool                        search_partitions_egold(uint32_t start_addr);
        bool                        search_partitions_sgold(uint32_t start_addr);
        bool                        search_partitions_sgold2(uint32_t start_addr);
        bool                        search_partitions_sgold2_elka(uint32_t start_addr);

        void                        detect_platform();

        void                        old_search_partitions_egold_ce();
        void                        old_search_partitions_sgold_sgold2();
        void                        old_search_partitions_sgold2_elka();

        void                        inspect();

        std::vector<uint32_t>       find_pattern8(const Patterns::Readable &pattern_readable, uint32_t start = 0, bool break_first = false);
        std::vector<uint32_t>       find_pattern(const Patterns::Readable &pattern_readable, uint32_t start = 0, bool break_first = false);

};


};
};

#endif
