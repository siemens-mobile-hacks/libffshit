#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_HASH_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_HASH_H

#include <cstdint>
#include <string>

namespace FULLFLASH {
namespace Filesystem {

// Every directory entry carries a hash of the entry's name, which the firmware looks names up
// by. Names are case-insensitive for it: the hash folds the case first.

// SGOLD2 and SGOLD2_ELKA: the UTF-16 name, case-folded to lower case
uint16_t name_hash_utf16(const std::u16string &name);
// SGOLD: the 8-bit name, upper-cased
uint16_t name_hash_8bit(const std::string &name);

// What the hashes fold the case with: two names the same after folding are the same name
char16_t fold_case_utf16(char16_t c);
char     fold_case_8bit(char c);

};
};

#endif
