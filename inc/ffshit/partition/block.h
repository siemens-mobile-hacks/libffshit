#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_BLOCK_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_BLOCK_H

#include "ffshit/rawdata.h"

#include <cstdint>
#include <vector>

namespace FULLFLASH {
namespace Partitions {

class Block {
    public:
        struct Header {
            Header() = default;

            char        name[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
            uint16_t    unknown_1 = 0x0;
            uint16_t    unknown_2 = 0x0;
            uint32_t    unknown_3 = 0x0;

            //x55
            uint16_t    unknown_4 = 0x0;
        };

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
