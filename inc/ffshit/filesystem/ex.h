#ifndef FULLFLASH_FILESYSTEM_EXCEPTION_H
#define FULLFLASH_FILESYSTEM_EXCEPTION_H

#include <fmt/format.h>
#include <string>

namespace FULLFLASH {
namespace Filesystem {

class Exception {
    public:
        template<typename Format, typename ...Args>
        Exception(Format format, const Args& ...args) {
            this->msg = fmt::format(format, args...);
        }

        const std::string &what() const {
            return msg;
        }

        const char *what_c() const {
            return msg.c_str();
        }

    private:
        std::string msg;
};
    
};
};

#endif
