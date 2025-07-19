#include "ffshit/fullflash.h"
#include "ffshit/log/logger.h"
#include "ffshit/ex.h"

namespace FULLFLASH {

FULLFLASH::FULLFLASH(std::filesystem::path fullflash_path) {
    std::ifstream file;

    file.open(fullflash_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw Exception("Couldn't open file '{}': {}\n", fullflash_path.string(), std::string(strerror(errno)));
    }

    file.seekg(0, std::ios_base::end);
    size_t data_size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    this->data = RawData(file, 0, data_size);

    x65flasher_fix();

    this->detector = Detector::build(this->data);
}

FULLFLASH::FULLFLASH(char *ff_data, size_t ff_data_size) {
    this->data      = RawData(ff_data, ff_data_size);

    x65flasher_fix();

    this->detector = Detector::build(this->data);
}

FULLFLASH::FULLFLASH(std::filesystem::path fullflash_path, Platform platform) {
    std::ifstream file;

    file.open(fullflash_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw Exception("Couldn't open file '{}': {}\n", fullflash_path.string(), std::string(strerror(errno)));
    }

    file.seekg(0, std::ios_base::end);
    size_t data_size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    this->data = RawData(file, 0, data_size);

    x65flasher_fix();

    this->detector = Detector::build(this->data, platform);
}

FULLFLASH::FULLFLASH(char *ff_data, size_t ff_data_size, Platform platform) {
    this->data      = RawData(ff_data, ff_data_size);

    x65flasher_fix();

    this->detector = Detector::build(this->data, platform);
}

void FULLFLASH::load_partitions(bool old_search_algorithm, uint32_t search_start_addr) {
    this->partitions = Partitions::Partitions::build(data, detector, old_search_algorithm, search_start_addr);
}

const Detector &FULLFLASH::get_detector() const {
    return *detector;
}

Partitions::Partitions::Ptr FULLFLASH::get_partitions() const {
    return partitions;
}

void FULLFLASH::x65flasher_fix() {
    char FBK[3];

    data.read_type<char>(0x0, FBK, 3);

    bool x65_flasher_detected = false;

    if (FBK[0] == 'F' &&
        FBK[1] == 'B' &&
        FBK[2] == 'K') {
        x65_flasher_detected = true;
    }

    if (!x65_flasher_detected) {
        return;
    }

    Log::Logger::warn("x65flasher detected");

    RawData tmp = this->data;

    size_t old_size = tmp.get_size();
    size_t new_size = old_size - 0x10;

    this->data =  RawData(tmp, 0x10, new_size);

    Log::Logger::warn("x65flasher fixed {:08X} -> {:08X}", old_size, new_size);
}

};
