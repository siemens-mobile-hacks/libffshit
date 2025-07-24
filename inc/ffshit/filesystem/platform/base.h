#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_PLATFORM_BASE_H

#include <memory>
#include <fmt/format.h>

#include "ffshit/filesystem/structure/structure.h"


namespace FULLFLASH {
namespace Filesystem  {

class Base {
    public:
        using Ptr = std::shared_ptr<Base>;

        Base() :    verbose_processing(false),
                    verbose_headers(false),
                    verbose_data(false)
                    { }

        void                            log_verbose_processing(bool enabled)    { this->verbose_processing  = enabled; }
        void                            log_verbose_headers(bool enabled)       { this->verbose_headers     = enabled; }
        void                            log_verbose_data(bool enabled)          { this->verbose_data        = enabled; }

        virtual void                    load(bool skip_broken = false, bool skip_dup = false, std::vector<std::string> parts_to_extract = {}) = 0;
        virtual const Directory::Ptr    get_root() const = 0;

        virtual ~Base() { }

    protected:
        bool verbose_processing;
        bool verbose_headers;
        bool verbose_data;

    private:
};

static const std::string ROOT_NAME = "FFS";
static const std::string ROOT_PATH = fmt::format("/{}/", ROOT_NAME);

};
};

#endif
