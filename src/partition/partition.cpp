#include "ffshit/partition/partition.h"

namespace FULLFLASH {
namespace Partitions {

Partition::Partition() { }
Partition::Partition(const std::string name) : name(name) { }

void Partition::add_block(Block &&block) {
    this->blocks.emplace_back(block);
}

void Partition::add_block(const Block &block) {
    this->blocks.push_back(block);
}

const std::string &Partition::get_name() const {
    return name;
}

const Blocks &Partition::get_blocks() const {
    return blocks;
}

};
};
