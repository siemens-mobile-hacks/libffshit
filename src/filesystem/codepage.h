#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_CODEPAGE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_CODEPAGE_H

#include <string>

namespace FULLFLASH {
namespace Filesystem {

// SGOLD keeps a name in the 8-bit codepage of the phone's language when the codepage has all of its
// characters, and else as 0x1F followed by the name in UTF-8

// Throws when the codepage is unknown
void            check_codepage(const std::string &codepage);
// The name as an SGOLD header keeps it. Throws when the name is not UTF-8.
std::string     sgold_name_from_utf8(const std::string &name, const std::string &codepage);
// The name in UTF-8. A name the codepage cannot decode stays as it is.
std::string     sgold_name_to_utf8(const std::string &stored, const std::string &codepage);

};
};

#endif
