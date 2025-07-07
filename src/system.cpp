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

bool remove_directory(const std::filesystem::path path, std::error_code &error_code) {
    if (!std::filesystem::exists(path)) {
        return true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_directory(entry)) {
            if (!remove_directory(entry, error_code)) {
                return false;
            }
        } else {
            if (!std::filesystem::remove(entry, error_code)) {
                return false;
            }
        }
    }

    return std::filesystem::remove(path);
}

bool create_directory(const std::filesystem::path path, std::filesystem::perms perms, std::error_code &error_code) {
    if (std::filesystem::create_directory(path, error_code)) {
        std::filesystem::permissions(path, perms, std::filesystem::perm_options::replace, error_code);

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
