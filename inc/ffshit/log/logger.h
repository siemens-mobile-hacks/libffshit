#ifndef LIBFFSHIT_LOG_LOGGER_H
#define LIBFFSHIT_LOG_LOGGER_H

#include "ffshit/log/interface.h"

namespace FULLFLASH {
namespace Log {

class Logger {
    public:

        static void init(Interface::Ptr interface_ptr) {
            Log::Logger::get_instance(interface_ptr);
        }

        static Logger &get_instance(Interface::Ptr interface_ptr = nullptr) {
            static Logger log(interface_ptr);

            return log;
        }

        template<typename Format, typename ...Args>
        static void info(Format format, const Args& ...args) {
            Logger &log = Log::Logger::get_instance();

            std::string msg = fmt::format(format, args...);

            if (log.interface) {
                log.interface->on_info(msg);
            }
        }

        template<typename Format, typename ...Args>
        static void warn(Format format, const Args& ...args) {
            Logger &log = Log::Logger::get_instance();

            std::string msg = fmt::format(format, args...);

            if (log.interface) {
                log.interface->on_warning(msg);
            }
        }

        template<typename Format, typename ...Args>
        static void error(Format format, const Args& ...args) {
            Logger &log = Log::Logger::get_instance();

            std::string msg = fmt::format(format, args...);

            if (log.interface) {
                log.interface->on_error(msg);
            }
        }

        template<typename Format, typename ...Args>
        static  void debug(Format format, const Args& ...args) {
            Logger &log = Log::Logger::get_instance();

            std::string msg = fmt::format(format, args...);

            if (log.interface) {
                log.interface->on_debug(msg);
            }
        }
    private:
        Logger(Interface::Ptr log_interface) : interface(log_interface) {

        }

        Interface::Ptr interface;

};

};
};

#endif
