#ifndef LIBFFSHIT_LOG_INTERFACE_H
#define LIBFFSHIT_LOG_INTERFACE_H

#include <string>
#include <memory>

namespace FULLFLASH {
namespace Log {

class Interface {
    public:
        using Ptr = std::shared_ptr<Interface>;

        virtual void on_info(std::string msg) = 0;
        virtual void on_warning(std::string msg) = 0;
        virtual void on_error(std::string msg) = 0;
        virtual void on_debug(std::string msg) = 0;
};

};
};

#endif
