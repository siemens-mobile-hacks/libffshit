#include "ffshit/filesystem/hash.h"

#include <algorithm>
#include <iterator>

namespace FULLFLASH {
namespace Filesystem {

// Mixed into every character, by its low six bits
static const uint16_t HASH_TABLE[64] = {
    0x4080, 0x9C6A, 0x1952, 0xEC99, 0xE800, 0x50E3, 0xEE37, 0x63F6,
    0x8C55, 0x3088, 0x8D3F, 0x5116, 0xBAD4, 0x014F, 0xD5A8, 0xF387,
    0x421B, 0xFFA2, 0xE848, 0xE35D, 0x37A9, 0xA92F, 0x7DE7, 0x18C0,
    0x2F28, 0x1A20, 0xDE68, 0x7A72, 0x65FB, 0xF707, 0xB908, 0x9387,
    0xC7D2, 0xAA77, 0x9FE4, 0x57B9, 0xF4DA, 0x04DB, 0x2D6E, 0x8DA1,
    0x65A0, 0xB33C, 0x502B, 0x6BB4, 0x8CAE, 0x2EED, 0x0374, 0xB3B2,
    0x5F96, 0xA87D, 0xF276, 0x8597, 0x0A06, 0x7473, 0x7043, 0xC9AB,
    0x12C4, 0xDBDF, 0x93B6, 0x5F6B, 0xDCFF, 0x1EE6, 0x93EE, 0xC687,
};

// The sum of the characters' values, modulo a prime
static constexpr uint32_t HASH_MODULUS = 64567;

// The case folding the NewSGOLD firmwares apply to UTF-16 names: Unicode's simple case folding
// of Latin, Greek, Cyrillic, Armenian, Latin Extended Additional, Greek Extended, the Roman
// numerals, the circled letters and the fullwidth forms. Every character of a run from `first`
// to `last`, `step` apart, maps to itself plus `delta`.
struct FoldingRun {
    uint16_t    first;
    uint16_t    last;
    uint16_t    step;
    int16_t     delta;
};

static const FoldingRun CASE_FOLDING[] = {
    { 0x0041, 0x005A, 1, 32 },
    { 0x00B5, 0x00B5, 1, 775 },
    { 0x00C0, 0x00D6, 1, 32 },
    { 0x00D8, 0x00DE, 1, 32 },
    { 0x0100, 0x012E, 2, 1 },
    { 0x0132, 0x0136, 2, 1 },
    { 0x0139, 0x0147, 2, 1 },
    { 0x014A, 0x0176, 2, 1 },
    { 0x0178, 0x0178, 1, -121 },
    { 0x0179, 0x017D, 2, 1 },
    { 0x017F, 0x017F, 1, -268 },
    { 0x0181, 0x0181, 1, 210 },
    { 0x0182, 0x0184, 2, 1 },
    { 0x0186, 0x0186, 1, 206 },
    { 0x0187, 0x0187, 1, 1 },
    { 0x0189, 0x018A, 1, 205 },
    { 0x018B, 0x018B, 1, 1 },
    { 0x018E, 0x018E, 1, 79 },
    { 0x018F, 0x018F, 1, 202 },
    { 0x0190, 0x0190, 1, 203 },
    { 0x0191, 0x0191, 1, 1 },
    { 0x0193, 0x0193, 1, 205 },
    { 0x0194, 0x0194, 1, 207 },
    { 0x0196, 0x0196, 1, 211 },
    { 0x0197, 0x0197, 1, 209 },
    { 0x0198, 0x0198, 1, 1 },
    { 0x019C, 0x019C, 1, 211 },
    { 0x019D, 0x019D, 1, 213 },
    { 0x019F, 0x019F, 1, 214 },
    { 0x01A0, 0x01A4, 2, 1 },
    { 0x01A6, 0x01A6, 1, 218 },
    { 0x01A7, 0x01A7, 1, 1 },
    { 0x01A9, 0x01A9, 1, 218 },
    { 0x01AC, 0x01AC, 1, 1 },
    { 0x01AE, 0x01AE, 1, 218 },
    { 0x01AF, 0x01AF, 1, 1 },
    { 0x01B1, 0x01B2, 1, 217 },
    { 0x01B3, 0x01B5, 2, 1 },
    { 0x01B7, 0x01B7, 1, 219 },
    { 0x01B8, 0x01B8, 1, 1 },
    { 0x01BC, 0x01BC, 1, 1 },
    { 0x01C4, 0x01C4, 1, 2 },
    { 0x01C5, 0x01C5, 1, 1 },
    { 0x01C7, 0x01C7, 1, 2 },
    { 0x01C8, 0x01C8, 1, 1 },
    { 0x01CA, 0x01CA, 1, 2 },
    { 0x01CB, 0x01DB, 2, 1 },
    { 0x01DE, 0x01EE, 2, 1 },
    { 0x01F1, 0x01F1, 1, 2 },
    { 0x01F2, 0x01F4, 2, 1 },
    { 0x01F6, 0x01F6, 1, -97 },
    { 0x01F7, 0x01F7, 1, -56 },
    { 0x01F8, 0x021E, 2, 1 },
    { 0x0220, 0x0220, 1, -130 },
    { 0x0222, 0x0232, 2, 1 },
    { 0x0345, 0x0345, 1, 116 },
    { 0x0386, 0x0386, 1, 38 },
    { 0x0388, 0x038A, 1, 37 },
    { 0x038C, 0x038C, 1, 64 },
    { 0x038E, 0x038F, 1, 63 },
    { 0x0391, 0x03A1, 1, 32 },
    { 0x03A3, 0x03AB, 1, 32 },
    { 0x03C2, 0x03C2, 1, 1 },
    { 0x03D0, 0x03D0, 1, -30 },
    { 0x03D1, 0x03D1, 1, -25 },
    { 0x03D5, 0x03D5, 1, -15 },
    { 0x03D6, 0x03D6, 1, -22 },
    { 0x03D8, 0x03EE, 2, 1 },
    { 0x03F0, 0x03F0, 1, -54 },
    { 0x03F1, 0x03F1, 1, -48 },
    { 0x03F4, 0x03F4, 1, -60 },
    { 0x03F5, 0x03F5, 1, -64 },
    { 0x03F7, 0x03F7, 1, 1 },
    { 0x03F9, 0x03F9, 1, -7 },
    { 0x03FA, 0x03FA, 1, 1 },
    { 0x0400, 0x040F, 1, 80 },
    { 0x0410, 0x042F, 1, 32 },
    { 0x0460, 0x0480, 2, 1 },
    { 0x048A, 0x04BE, 2, 1 },
    { 0x04C1, 0x04CD, 2, 1 },
    { 0x04D0, 0x04F4, 2, 1 },
    { 0x04F8, 0x04F8, 1, 1 },
    { 0x0500, 0x050E, 2, 1 },
    { 0x0531, 0x0556, 1, 48 },
    { 0x1E00, 0x1E94, 2, 1 },
    { 0x1E9B, 0x1E9B, 1, -58 },
    { 0x1EA0, 0x1EF8, 2, 1 },
    { 0x1F08, 0x1F0F, 1, -8 },
    { 0x1F18, 0x1F1D, 1, -8 },
    { 0x1F28, 0x1F2F, 1, -8 },
    { 0x1F38, 0x1F3F, 1, -8 },
    { 0x1F48, 0x1F4D, 1, -8 },
    { 0x1F59, 0x1F5F, 2, -8 },
    { 0x1F68, 0x1F6F, 1, -8 },
    { 0x1FB8, 0x1FB9, 1, -8 },
    { 0x1FBA, 0x1FBB, 1, -74 },
    { 0x1FBE, 0x1FBE, 1, -7173 },
    { 0x1FC8, 0x1FCB, 1, -86 },
    { 0x1FD8, 0x1FD9, 1, -8 },
    { 0x1FDA, 0x1FDB, 1, -100 },
    { 0x1FE8, 0x1FE9, 1, -8 },
    { 0x1FEA, 0x1FEB, 1, -112 },
    { 0x1FEC, 0x1FEC, 1, -7 },
    { 0x1FF8, 0x1FF9, 1, -128 },
    { 0x1FFA, 0x1FFB, 1, -126 },
    { 0x2126, 0x2126, 1, -7517 },
    { 0x212A, 0x212A, 1, -8383 },
    { 0x212B, 0x212B, 1, -8262 },
    { 0x2160, 0x216F, 1, 16 },
    { 0x24B6, 0x24CF, 1, 26 },
    { 0xFF21, 0xFF3A, 1, 32 },
};

template<typename Iterator, typename Fold>
static uint16_t hash(Iterator begin, Iterator end, Fold fold) {
    uint32_t sum        = 0;
    uint32_t previous   = 0;

    for (Iterator it = begin; it != end; ++it) {
        uint32_t c = fold(*it);

        sum         += HASH_TABLE[c & 0x3F] ^ (c << (previous & 7));
        previous    = c;
    }

    uint16_t result = sum % HASH_MODULUS;

    return result ? result : 1;
}

char16_t fold_case_utf16(char16_t c) {
    auto it = std::upper_bound(std::begin(CASE_FOLDING), std::end(CASE_FOLDING), c, [](char16_t c, const FoldingRun &run) {
        return c < run.first;
    });

    if (it == std::begin(CASE_FOLDING)) {
        return c;
    }

    const FoldingRun &run = *std::prev(it);

    if (c > run.last || (c - run.first) % run.step) {
        return c;
    }

    return static_cast<char16_t>(c + run.delta);
}

char fold_case_8bit(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }

    return c;
}

uint16_t name_hash_utf16(const std::u16string &name) {
    return hash(name.begin(), name.end(), fold_case_utf16);
}

uint16_t name_hash_8bit(const std::string &name) {
    return hash(name.begin(), name.end(), [](char c) {
        return static_cast<uint8_t>(fold_case_8bit(c));
    });
}

};
};
