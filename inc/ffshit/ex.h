#ifndef LIBFFSHIT_FULLFLASH_EXCEPTION_H
#define LIBFFSHIT_FULLFLASH_EXCEPTION_H

#include <string>

#include <fmt/format.h>

namespace FULLFLASH {

class BaseException {
    public:
        virtual const std::string & what() const = 0;
        virtual const char *        what_c() const = 0;

        virtual ~BaseException() {}
};

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

#endif
