#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITIONS_H

#include "ffshit/rawdata.h"
#include "ffshit/filesystem/platform/types.h"

#include "ffshit/partition/partition.h"

#include <memory>
#include <map>

namespace FULLFLASH {
namespace Partitions {

class Partitions {
    public:
        using Ptr = std::shared_ptr<Partitions>;
        using Map = std::map<std::string, Partition>;
        
        template<typename ...Args>
        static Ptr build(Args ...args) {
            return std::make_unique<Partitions>(args...);
        }

        Partitions(std::string fullflash_path);
        Partitions(std::string fullflash_path, Platform platform);

        const Map &                 get_partitions() const;

        const Platform              get_platform() const;
        const std::string &         get_imei() const;
        const std::string &         get_model() const;

    private:
        uint32_t                    block_size;
        Map                         partitions_map;
        
        RawData                     data;
        Platform                    platform;
        std::string                 imei;
        std::string                 model;

        void                        detect_platform();
        void                        search_blocks();
        void                        search_blocks_x85();


};


};
};

#endif
