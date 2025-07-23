#include "ffshit/detector.h"
#include "ffshit/log/logger.h"
#include "ffshit/system.h"

#include <algorithm>

namespace FULLFLASH {

Detector::Detector(const RawData &data) : data(data) {
    sl75_bober_kurwa    = false;
    base_adress         = 0;

    detect_platform();
    detect_imei_model();
}

Detector::Detector(const RawData &data, Platform platform) : data(data) {
    sl75_bober_kurwa    = false;
    base_adress         = 0;

    this->platform = platform;

    detect_imei_model();
}

const Platform Detector::get_platform() const {
    return platform;
}

const std::string &Detector::get_model() const {
    return model;
}

const std::string &Detector::get_imei() const {
    return imei;
}

const size_t Detector::get_base_address() const {
    return base_adress;
}

const bool Detector::is_sl75() const {
    return sl75_bober_kurwa;
}

bool Detector::check_string(const std::string &str) {
    bool valid = true;

    std::for_each(str.cbegin(), str.cend(), [&](char c) {
        if (!isprint(c)) {
            valid = false;
        }
    });

    return valid;
}

void Detector::detect_platform() {
    data.read_string(BC65_BC75_OFFSET, bcore_name, 1);

    if (bcore_name == "BC65" || bcore_name == "BCORE65") {
        platform    = Platform::SGOLD;
        base_adress = 0xA0000000;
    } else if (bcore_name == "BC75") {
        platform    = Platform::SGOLD2;
        base_adress = 0xA0000000;
    } else {
        bcore_name.clear();

        data.read_string(BC85_OFFSET, bcore_name, 1);

        if (bcore_name == "BC85") {
            platform    = Platform::SGOLD2_ELKA;
            base_adress = 0xA0000000;
        } else {
            platform = Platform::UNK;
        }
    }

    if (platform == Platform::UNK) {
        for (const auto &offset : EGOLD_INFO_OFFSETS) {
            std::string magick;

            data.read_string(offset + EGOLD_MAGICK_SIEMENS_OFFSET, magick);

            if (magick != "SIEMENS") {
                continue;
            }

            platform        = Platform::EGOLD_CE;
            base_adress     = 16777216 - data.get_size(); // Max adress size
            egold_offset = offset;

            break;
        }
    }
}

void Detector::detect_imei_model() {
    switch (platform) {
        case Platform::EGOLD_CE: {
            data.read_string(egold_offset + EGOLD_MODEL_OFFSET, model);

            break;
        }

        case Platform::SGOLD: {
            data.read_string(X65_MODEL_OFFSET, model);
            data.read_string(X65_IMEI_OFFSET, imei);

            if (imei.length() != 15) {
                imei.clear();
                data.read_string(X65_7X_IMEI_OFFSET, imei);
            };

            if (!check_string(imei)) {
                imei.clear();
                data.read_string(X65_7X_IMEI_OFFSET, imei);
            }

            break;
        }

        case Platform::SGOLD2: {
            data.read_string(X75_MODEL_OFFSET, model);
            data.read_string(X75_IMEI_OFFSET, imei);

            std::string model_local(model);

            System::to_lower(model_local);

            if (model_local.find("sl75") != std::string::npos) {
                sl75_bober_kurwa = true;
            } else if (data.get_size() > 0x04000000) {
                sl75_bober_kurwa = true;
            }

            break;
        }

        case Platform::SGOLD2_ELKA: {
            data.read_string(X85_MODEL_OFFSET, model);
            data.read_string(X85_IMEI_OFFSET, imei);

            if (!(check_string(model) & check_string(imei))) {
                imei.clear();

                data.read_string(X85_MODEL_OFFSET + 0x10, model);
                data.read_string(X85_IMEI_OFFSET + 0x10, imei);
            }

            break;
        }

        case Platform::UNK: {
            break;
        }
    }

    bool borken_imei = false;
    bool broken_model = false;

    if (imei.size() != 15) {
        borken_imei = true;
    } else {
        borken_imei = !check_string(imei);
    }

    broken_model = !check_string(model);

    if (borken_imei) {
        Log::Logger::warn("Couldn't detect IMEI");
        Log::Logger::debug("IMEI broken: {}", imei);

        imei.clear();
    }

    if (broken_model) {
        Log::Logger::warn("Couldn't detect model");

        model = bcore_name;
    }
}

};
