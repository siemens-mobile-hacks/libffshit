#include "ffshit/filesystem/help.h"

namespace FULLFLASH {
namespace Filesystem {

//tnx perk11
TimePoint fat_timestamp_to_unix(uint32_t fat_time) {
    uint32_t year   = 1980 + (fat_time >> 25);
    uint32_t month  = (fat_time >> 21) & 0x0F;
    uint32_t day    = (fat_time >> 16) & 0x1F;
    uint32_t hour   = (fat_time >> 11) & 0x1F;
    uint32_t mins   = (fat_time >>  5) & 0x3F;
    uint32_t secs   = (fat_time & 0x1F) * 2;

    std::tm tm{};

    tm.tm_year  = year - 1900;
    tm.tm_mon   = month - 1;
    tm.tm_mday  = day;
    tm.tm_hour  = hour;
    tm.tm_min   = mins;
    tm.tm_sec   = secs;

    time_t unix_timestamp = std::mktime(&tm);

    return std::chrono::system_clock::from_time_t(unix_timestamp);
}

};
};
