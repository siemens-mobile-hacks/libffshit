#ifndef LIBFFSHIT_FULLFLASH_FULLFLASH_H
#define LIBFFSHIT_FULLFLASH_FULLFLASH_H

#include "ffshit/detector.h"
#include "ffshit/partition/partitions.h"
#include "ffshit/rawdata.h"

#include <filesystem>
#include <memory>

namespace FULLFLASH {

class FULLFLASH {
    public:
        using Ptr = std::shared_ptr<FULLFLASH>;

        FULLFLASH(std::filesystem::path fullflash_path);
        FULLFLASH(char *ff_data, size_t ff_data_size);

        FULLFLASH(std::filesystem::path fullflash_path, Platform platform);
        FULLFLASH(char *ff_data, size_t ff_data_size, Platform platform);

        template<typename ...Args>
        static Ptr build(Args... args) {
            return std::make_shared<FULLFLASH>(args...);
        }

        void                                load_partitions(bool old_search_algorithm, uint32_t search_start_addr);

        const Detector &                    get_detector() const;
        Partitions::Partitions::Ptr         get_partitions() const;

    private:
        RawData                             data;
        Detector::Ptr                       detector;
        Partitions::Partitions::Ptr         partitions;

        void                                x65flasher_fix();

};

};

#endif
