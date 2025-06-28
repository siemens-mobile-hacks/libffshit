#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_BLOCK_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_BLOCK_H

#include "ffshit/rawdata.h"

#include <cstdint>
#include <vector>

namespace FULLFLASH {
namespace Partitions {

class Block {
    public:
        typedef struct {
            char        name[8];
            uint16_t    unknown_1;
            uint16_t    unknown_2;
            uint32_t    unknown_3;

            //x55
            uint16_t    unknown_4;

        } Header;

        Block(const Header &header, const RawData & data, uint32_t addr, uint32_t size);

        const Header &  get_header() const;
        const uint32_t  get_addr()  const;
        const uint32_t  get_size()  const;
        const RawData & get_data()  const;

    private:
        Header      header;

        uint32_t    addr;
        uint32_t    size;

        RawData     data;
};

using Blocks = std::vector<Block>;

};
};

#endif
