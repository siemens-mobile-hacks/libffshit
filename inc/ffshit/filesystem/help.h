#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_HELP_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_HELP_H

#include <ctime>
#include <cstdint>
#include <chrono>

namespace FULLFLASH {
namespace Filesystem {

using TimePoint = std::chrono::system_clock::time_point;

//tnx perk11
TimePoint fat_timestamp_to_unix(uint32_t fat_time);

};
};

#endif
