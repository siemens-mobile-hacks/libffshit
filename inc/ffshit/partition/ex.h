#ifndef LIBFFSHIT_FULLFLASH_PARTITIONS_EXCEPTION_H
#define LIBFFSHIT_FULLFLASH_PARTITIONS_EXCEPTION_H

#include <fmt/format.h>
#include <string>

#include "ffshit/ex.h"

namespace FULLFLASH {
namespace Partitions {

class Exception : public BaseException {
    public:
        template<typename Format, typename ...Args>
        Exception(Format format, const Args& ...args) {
            this->msg = fmt::format(format, args...);
        }

        const std::string &what() const override final {
            return msg;
        }

        const char *what_c() const override final {
            return msg.c_str();
        }

    private:
        std::string msg;
};

};
};

#endif
