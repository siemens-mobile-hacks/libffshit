#ifndef FULLFLASH_EXCEPTION_H
#define FULLFLASH_EXCEPTION_H

#include <string>

#include <fmt/format.h>

namespace FULLFLASH {

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

#endif
