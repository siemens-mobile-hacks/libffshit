#include "filesystem/codepage.h"

#include "ffshit/filesystem/ex.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

#include <iconv.h>

namespace FULLFLASH {
namespace Filesystem {

static constexpr char UTF8_NAME_PREFIX = 0x1F;

static iconv_t open_iconv(const std::string &to_code, const std::string &from_code) {
    iconv_t iccd = iconv_open(to_code.c_str(), from_code.c_str());

    if (iccd == ((iconv_t)-1)) {
        throw Exception("Can't convert from {} to {}: {}", from_code, to_code, strerror(errno));
    }

    return iccd;
}

// Whether the whole input converts, and what to
static bool convert(const std::string &to_code, const std::string &from_code, const std::string &input, std::string &output) {
    iconv_t     iccd = open_iconv(to_code, from_code);
    std::string buffer(input.size() * 4 + 4, '\0');

#ifdef __MINGW32__
    const char *inptr = input.data();
#else
    char *      inptr = const_cast<char *>(input.data());
#endif
    char *      outptr      = &buffer[0];
    size_t      in_left     = input.size();
    size_t      out_left    = buffer.size();

    size_t      result      = iconv(iccd, &inptr, &in_left, &outptr, &out_left);

    iconv_close(iccd);

    // glibc fails on a character the other side lacks, musl puts in '*' and counts it
    if (result != 0 || in_left != 0) {
        return false;
    }

    output = buffer.substr(0, buffer.size() - out_left);

    return true;
}

static bool is_ascii(const std::string &name) {
    return std::all_of(name.begin(), name.end(), [](char c) {
        return static_cast<unsigned char>(c) < 0x80;
    });
}

void check_codepage(const std::string &codepage) {
    try {
        iconv_close(open_iconv(codepage, "UTF-8"));
        iconv_close(open_iconv("UTF-8", codepage));
    } catch (const Exception &) {
        throw Exception("Unknown codepage {}", codepage);
    }
}

std::string sgold_name_from_utf8(const std::string &name, const std::string &codepage) {
    std::string stored;

    if (is_ascii(name)) {
        return name;
    }

    if (convert(codepage, "UTF-8", name, stored)) {
        return stored;
    }

    if (!convert("UTF-16LE", "UTF-8", name, stored)) {
        throw Exception("'{}' is not UTF-8", name);
    }

    return UTF8_NAME_PREFIX + name;
}

std::string sgold_name_to_utf8(const std::string &stored, const std::string &codepage) {
    std::string name;

    if (stored.size() >= 2 && stored[0] == UTF8_NAME_PREFIX) {
        return stored.substr(1);
    }

    if (is_ascii(stored) || !convert("UTF-8", codepage, stored, name)) {
        return stored;
    }

    return name;
}

};
};
