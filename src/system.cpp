#include "ffshit/system.h"

#include <algorithm>

namespace System {

bool is_file_exists(const std::filesystem::path path) {
    if (std::filesystem::exists(path)) {
        if (std::filesystem::is_regular_file(path)) {
            return true;
        }
    }

    return false;
}

bool is_directory_exists(const std::filesystem::path path) {
    if (std::filesystem::exists(path)) {
        if (std::filesystem::is_directory(path)) {
            return true;
        }
    }

    return false;
}

bool remove_directory(const std::filesystem::path path) {
    if (!std::filesystem::exists(path)) {
        return true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_directory(entry)) {
            if (!remove_directory(entry)) {
                return false;
            }
        } else {
            if (!std::filesystem::remove(entry)) {
                return false;
            }
        }
    }

    return std::filesystem::remove(path);
}


bool create_directory(const std::filesystem::path path, std::filesystem::perms perms) {
    if (std::filesystem::create_directory(path)) {
        std::filesystem::permissions(path, perms, std::filesystem::perm_options::replace);

        return true;
    }

    return false;
}

void to_lower(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ 
        return std::tolower(c); 
    });
}

void to_upper(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ 
        return std::toupper(c); 
    });
}

};
