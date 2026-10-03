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
// Rounds down to even seconds, and into the years 1980 to 2107
uint32_t  unix_to_fat_timestamp(const TimePoint &time_point);

};
};

#endif
