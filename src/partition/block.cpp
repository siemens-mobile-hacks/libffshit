#include "ffshit/partition/block.h"
#include "ffshit/partition/ex.h"

namespace FULLFLASH {
namespace Partitions {

Block::Block(const Header &header, const RawData &data, uint32_t addr, uint32_t size) :
    header(header),
    data(data),
    addr(addr),
    size(size) {
}

const Block::Header &Block::get_header() const {
    return header;
}

const uint32_t Block::get_addr()  const {
    return addr;
}

const uint32_t Block::get_size()  const {
    return size;
}

const RawData &Block::get_data()  const {
    return data;
}

};
};
