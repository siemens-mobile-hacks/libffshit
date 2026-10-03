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
    // The phone's clock shows the local time, daylight saving time included
    tm.tm_isdst = -1;

    time_t unix_timestamp = std::mktime(&tm);

    return std::chrono::system_clock::from_time_t(unix_timestamp);
}

uint32_t unix_to_fat_timestamp(const TimePoint &time_point) {
    time_t  unix_timestamp  = std::chrono::system_clock::to_time_t(time_point);
    std::tm tm              = *std::localtime(&unix_timestamp);

    uint32_t year = tm.tm_year + 1900;

    // What the format holds
    if (year < 1980) {
        return (1 << 21) | (1 << 16);
    }

    if (year > 2107) {
        return (127u << 25) | (12 << 21) | (31 << 16) | (23 << 11) | (59 << 5) | 29;
    }

    return  ((year - 1980) << 25) |
            ((tm.tm_mon + 1) << 21) |
            (tm.tm_mday << 16) |
            (tm.tm_hour << 11) |
            (tm.tm_min << 5) |
            (tm.tm_sec / 2);
}

};
};
