#ifndef LIBFFSHIT_FULLFLASH_DETECTOR_H
#define LIBFFSHIT_FULLFLASH_DETECTOR_H

#include "ffshit/filesystem/platform/types.h"
#include "ffshit/rawdata.h"

#include <vector>
#include <cstdint>

namespace FULLFLASH {

static constexpr size_t     BC65_BC75_OFFSET    = 0x870;
static constexpr size_t     BC85_OFFSET         = 0xC70;

static constexpr size_t     X65_MODEL_OFFSET    = 0x210;
static constexpr size_t     X75_MODEL_OFFSET    = 0x210;
static constexpr size_t     X85_MODEL_OFFSET    = 0x3E000;

static constexpr size_t     X65_IMEI_OFFSET     = 0x65C;
static constexpr size_t     X65_7X_IMEI_OFFSET  = 0x660;
static constexpr size_t     X75_IMEI_OFFSET     = 0x660;
static constexpr size_t     X85_IMEI_OFFSET     = 0x3E410;

static constexpr uint32_t   FF_ADDRESS_MASK     = 0x0FFFFFFF;

static constexpr size_t     EGOLD_INFO_OFFSET1          = 0x400300;
static constexpr size_t     EGOLD_INFO_OFFSET2          = 0x600300;
static constexpr size_t     EGOLD_INFO_OFFSET3          = 0x800300;

static std::vector<size_t>  EGOLD_INFO_OFFSETS = { EGOLD_INFO_OFFSET1, EGOLD_INFO_OFFSET2, EGOLD_INFO_OFFSET3 };

static constexpr size_t     EGOLD_MODEL_OFFSET          = 0x0C;
static constexpr size_t     EGOLD_MAGICK_SIEMENS_OFFSET = 0x1C;

class Detector {
    public:
        using Ptr = std::shared_ptr<Detector>;

        Detector(const RawData &data);
        Detector(const RawData &data, Platform platform);

        static Ptr build(const RawData &data) {
            return std::make_shared<Detector>(data);
        }

        static Ptr build(const RawData &data, Platform platform) {
            return std::make_shared<Detector>(data, platform);
        }

        const Platform      get_platform() const;
        const std::string & get_model() const;
        const std::string & get_imei() const;
        const bool          is_sl75() const;

    private:
        const RawData & data;

        Platform        platform;

        std::string     bcore_name;
        std::string     imei;
        std::string     model;

        size_t          egold_offset;

        bool            sl75_bober_kurwa;

        bool            check_string(const std::string &str);

        void            detect_platform();
        void            detect_imei_model();

};

};

#endif
