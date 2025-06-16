#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITION_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_PARTITION_H

#include <cstdint>
#include <string>

#include "ffshit/partition/block.h"

namespace FULLFLASH {
namespace Partitions {

class Partition {
    public:
        Partition();
        Partition(const std::string name);

        void                add_block(Block &&block);
        void                add_block(const Block &block);

        const std::string & get_name() const;
        const Blocks &      get_blocks() const;

    private:
        std::string name;
        Blocks      blocks;
};

};
};

#endif
